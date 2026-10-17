#pragma once
#include <QCryptographicHash>
#include <QEventLoop>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "../controlbar/RequestObserver.h"

// Loopback-only engine fixtures. Distinct ports provide distinct origins without DNS/network dependencies.
inline bool runNetworkBoundaryTests() {
    QTcpServer source, target;
    if (!source.listen(QHostAddress::LocalHost) || !target.listen(QHostAddress::LocalHost)) return false;
    const QString sourceOrigin = QString("http://127.0.0.1:%1").arg(source.serverPort());
    const QString targetOrigin = QString("http://127.0.0.1:%1").arg(target.serverPort());
    QMap<QByteArray, int> requests;
    bool omittedCookie = true, includedCookie = false;
    int handshakes = 0;
    auto accept = [&](QTcpServer &server, bool isTarget) {
        QObject::connect(&server, &QTcpServer::newConnection, &server, [&, isTarget, serverPtr=&server] {
            while (auto socket = serverPtr->nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket, isTarget] {
                    if (socket->property("handled").toBool()) { socket->readAll(); return; }
                    auto data = socket->property("headers").toByteArray() + socket->readAll();
                    socket->setProperty("headers", data);
                    if (!data.contains("\r\n\r\n")) return;
                    socket->setProperty("handled", true);
                    const auto parts = data.left(data.indexOf("\r\n")).split(' ');
                    if (parts.size() < 2) { socket->abort(); return; }
                    const auto method = parts[0], path = parts[1];
                    QMap<QByteArray,QByteArray> headers;
                    for (auto line : data.split('\n')) {
                        const int colon = line.indexOf(':');
                        if (colon > 0) headers.insert(line.left(colon).trimmed().toLower(), line.mid(colon+1).trimmed());
                    }
                    if (isTarget) requests[method + " " + path]++;
                    if (isTarget && path == "/socket" && headers.value("upgrade").toLower() == "websocket") {
                        ++handshakes;
                        const auto key = headers.value("sec-websocket-key");
                        const auto digest = QCryptographicHash::hash(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", QCryptographicHash::Sha1).toBase64();
                        socket->write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + digest + "\r\n\r\n");
                        return;
                    }
                    if (path == "/omit") omittedCookie = !headers.value("cookie").contains("cybou_boundary");
                    if (path == "/credential") includedCookie = headers.value("cookie").contains("cybou_boundary=present");
                    QByteArray extra;
                    if (!isTarget) extra += "Set-Cookie: cybou_boundary=present; Path=/; SameSite=Lax\r\n";
                    if (isTarget && path != "/deny" && path != "/preflight-deny") {
                        extra += "Access-Control-Allow-Origin: *\r\n";
                        if (method == "OPTIONS") extra += "Access-Control-Allow-Methods: PUT\r\nAccess-Control-Allow-Headers: X-Probe\r\n";
                    }
                    const QByteArray body = isTarget ? QByteArray("boundary-token") : QByteArray("<!doctype html><title>Boundary ready</title>");
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n" + extra + "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    };
    accept(source, false); accept(target, true);
    QWebEngineProfile profile;
    RequestObserver interceptor(&profile);
    interceptor.setHttpsOnly(false);
    profile.setUrlRequestInterceptor(&interceptor);
    QWebEnginePage page(&profile);
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    bool passed = false;
    int phase = 0;
    auto fail = [&](const QString &why) { qCritical() << "NETWORK_BOUNDARY_TEST FAIL" << phase << why; loop.quit(); };
    auto websocket = [&](const QString &label) {
        page.runJavaScript(QString(R"JS((()=>{const s=new WebSocket('%1/socket');s.onopen=()=>{document.title='%2-open';s.close()};s.onerror=()=>document.title='%2-blocked'})())JS")
                           .arg(QString(targetOrigin).replace("http:","ws:"), label));
    };
    QObject::connect(&deadline, &QTimer::timeout, &loop, [&] { fail("timeout"); });
    QObject::connect(&page, &QWebEnginePage::titleChanged, &loop, [&](const QString &title) {
        if (title.startsWith("boundary-fail")) { fail(title); return; }
        if (phase == 0 && title == "Boundary ready") {
            phase = 1;
            page.runJavaScript(QString(R"JS((async()=>{
                const target='%1';
                function check(v,m){if(!v)throw Error(m)}
                async function denied(path,options){try{await fetch(target+path,options);return false}catch(e){return true}}
                try {
                    check(document.cookie.includes('cybou_boundary=present'),'source cookie missing');
                    check((await (await fetch(target+'/allow')).text())==='boundary-token','allowed response');
                    check(await denied('/deny'),'missing ACAO readable');
                    check((await (await fetch(target+'/preflight-allow',{method:'PUT',headers:{'X-Probe':'1'}})).text())==='boundary-token','preflight allowed');
                    check(await denied('/preflight-deny',{method:'PUT',headers:{'X-Probe':'1'}}),'preflight denied');
                    check((await (await fetch(target+'/omit',{credentials:'omit'})).text())==='boundary-token','omit response');
                    check(await denied('/credential',{credentials:'include'}),'wildcard credential response readable');
                    document.title='cors-pass';
                } catch(e) {document.title='boundary-fail-'+e.message}
            })())JS").arg(targetOrigin));
        } else if (phase == 1 && title == "cors-pass") {
            if (requests.value("OPTIONS /preflight-allow") != 1 || requests.value("PUT /preflight-allow") != 1
                || requests.value("OPTIONS /preflight-deny") != 1 || requests.value("PUT /preflight-deny") != 0
                || requests.value("GET /deny") != 1 || !omittedCookie || !includedCookie) {
                fail("CORS server observations or credentials incorrect"); return;
            }
            qInfo() << "CORS_BOUNDARY_TEST PASS: distinct origins, allowed/denied reads, real preflight allow/deny, omit cookies, wildcard credential denial";
            phase = 2; interceptor.setHttpsOnly(true); websocket("strict");
        } else if (phase == 2 && title.startsWith("strict-")) {
            if (title != "strict-blocked" || handshakes != 0) { fail("strict ws escaped"); return; }
            phase = 3; interceptor.setHttpsOnly(false); websocket("allowed");
        } else if (phase == 3 && title.startsWith("allowed-")) {
            if (title != "allowed-open" || handshakes != 1) { fail("ws positive control failed"); return; }
            phase = 4; interceptor.setHttpsOnly(true); websocket("restored");
        } else if (phase == 4 && title.startsWith("restored-")) {
            if (title != "restored-blocked" || handshakes != 1) { fail("restored policy allowed a new ws handshake"); return; }
            int blocked = 0;
            for (const auto &value : interceptor.sample()) {
                const auto event = value.toMap();
                if (event.value("origin").toString().startsWith("ws:") && event.value("reason") == "insecure-transport" && event.value("blocked").toBool()) ++blocked;
            }
            if (blocked != 2) { fail("missing websocket interceptor deny evidence"); return; }
            qInfo() << "WEBSOCKET_BOUNDARY_TEST PASS: strict ws denied before handshake, real positive 101 handshake, new ws denied after strict policy restored";
            passed = true; loop.quit();
        }
    });
    deadline.start(20000);
    page.load(QUrl(sourceOrigin + "/page"));
    loop.exec();
    deadline.stop();
    return passed;
}
