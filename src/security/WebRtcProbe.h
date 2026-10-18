#pragma once
#include <QEventLoop>
#include <QJsonDocument>
#include <QTcpServer>
#include <QNetworkDatagram>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include <QtWebEngineCore/QWebEngineSettings>

// Diagnostic only (ASVS 17.1.1 browser analogue): which ICE candidates does a page obtain from
// RTCPeerConnection without any STUN/TURN server, with WebRTCPublicInterfacesOnly on and off?
// Prints WEBRTC_PROBE_RESULT lines; exit 0 means the experiment ran, not that a boundary holds.
inline int runWebRtcProbe() {
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                socket->readAll();
                const QByteArray body("<!doctype html><title>webrtc</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    QWebEngineProfile profile; // off-the-record, no app interceptor
    int status = 0;
    // Remote-candidate vector: the page injects a loopback ICE candidate and the browser may send
    // STUN connectivity checks (UDP) to it. Counted on a plain UDP socket the test owns.
    QUdpSocket udp;
    int stunChecks = 0, otherDatagrams = 0;
    if (!udp.bind(QHostAddress::LocalHost, 0)) return 2;
    QObject::connect(&udp, &QUdpSocket::readyRead, &udp, [&] {
        while (udp.hasPendingDatagrams()) {
            const auto d = udp.receiveDatagram().data();
            if (d.size() >= 20 && d[0] == 0 && d[1] == 1) ++stunChecks; else ++otherDatagrams;
        }
    });
    for (const bool publicOnly : {false, true}) {
        QWebEnginePage page(&profile);
        page.settings()->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, publicOnly);
        QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&page, &QWebEnginePage::titleChanged, &loop, [&](const QString &t) { if (t == "done") loop.quit(); });
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
            if (!ok) { loop.quit(); return; }
            page.runJavaScript(QStringLiteral(R"JS((async()=>{
                const out={candidates:[],error:null};
                try{
                    const pc=new RTCPeerConnection({iceServers:[]});
                    pc.createDataChannel('probe');
                    pc.onicecandidate=e=>{if(e.candidate)out.candidates.push(e.candidate.candidate)};
                    await pc.setLocalDescription(await pc.createOffer());
                    await new Promise(r=>{pc.onicegatheringstatechange=()=>{if(pc.iceGatheringState==='complete')r()};setTimeout(r,6000)});
                    pc.close();
                }catch(e){out.error=String(e)}
                try{
                    const a=new RTCPeerConnection({iceServers:[]});
                    a.createDataChannel('x');
                    const offer=await a.createOffer();
                    const sdp=offer.sdp.replace(/\r\n$/,'')+'\r\na=candidate:1 1 udp 2122260223 127.0.0.1 %1 typ host\r\n';
                    const b=new RTCPeerConnection({iceServers:[]});
                    await b.setRemoteDescription({type:'offer',sdp});
                    await b.setLocalDescription(await b.createAnswer());
                    await new Promise(r=>setTimeout(r,5000));
                    out.remoteInjected=true;a.close();b.close();
                }catch(e){out.injectError=String(e)}
                try{
                    // Functional cost: can two peers in this page open a data channel with no TURN?
                    const p1=new RTCPeerConnection({iceServers:[]}), p2=new RTCPeerConnection({iceServers:[]});
                    p1.onicecandidate=e=>{if(e.candidate)p2.addIceCandidate(e.candidate).catch(()=>{})};
                    p2.onicecandidate=e=>{if(e.candidate)p1.addIceCandidate(e.candidate).catch(()=>{})};
                    const ch=p1.createDataChannel('c');
                    const opened=new Promise(r=>{ch.onopen=()=>r(true);setTimeout(()=>r(false),8000)});
                    await p1.setLocalDescription(await p1.createOffer());
                    await p2.setRemoteDescription(p1.localDescription);
                    await p2.setLocalDescription(await p2.createAnswer());
                    await p1.setRemoteDescription(p2.localDescription);
                    out.peerChannelOpen=await opened;p1.close();p2.close();
                }catch(e){out.peerError=String(e)}
                window.__r=JSON.stringify(out);document.title='done';
            })())JS").arg(udp.localPort()));
        });
        timeout.start(40000);
        page.load(QUrl(QString("http://127.0.0.1:%1/").arg(server.serverPort())));
        loop.exec();
        QString result; QEventLoop get;
        page.runJavaScript("window.__r", [&](const QVariant &v) { result = v.toString(); get.quit(); });
        QTimer::singleShot(3000, &get, &QEventLoop::quit); get.exec();
        qInfo().noquote() << "WEBRTC_PROBE_RESULT publicInterfacesOnly=" << publicOnly << result;
        qInfo() << "WEBRTC_PROBE_REMOTE_CANDIDATE publicInterfacesOnly=" << publicOnly << "stunChecksAtLoopback" << stunChecks << "otherDatagrams" << otherDatagrams;
        stunChecks = 0; otherDatagrams = 0;
        if (result.isEmpty()) status = 2;
    }
    return status;
}
