#pragma once
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFile>
#include <QSet>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTimer>
#include <QtWebEngineCore/QWebEngineCertificateError>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include <functional>
#include <memory>
#include "../controlbar/RequestObserver.h"

// Public, test-only key/certificate; never installed as a trust anchor.
class BoundaryTlsServer final : public QTcpServer {
public:
    QSslCertificate certificate;
    QSslKey key;
    int handshakes = 0;
    int closedSockets = 0;
    QSet<QSslSocket *> activeSockets;
    std::function<void()> onSocketClosed;
protected:
    void incomingConnection(qintptr descriptor) override {
        auto socket = new QSslSocket(this);
        socket->setLocalCertificate(certificate);
        socket->setPrivateKey(key);
        socket->setPeerVerifyMode(QSslSocket::VerifyNone); // Server does not request client certificates.
        socket->setProtocol(QSsl::TlsV1_2OrLater);
        if (!socket->setSocketDescriptor(descriptor)) { delete socket; return; }
        QObject::connect(socket, &QSslSocket::disconnected, socket, [this, socket] {
            if (activeSockets.remove(socket)) {
                ++closedSockets;
                if (onSocketClosed) onSocketClosed();
            }
            socket->deleteLater();
        });
        QObject::connect(socket, &QSslSocket::readyRead, socket, [this, socket] {
            const auto data = socket->readAll();
            if (socket->property("upgraded").toBool()) {
                // Complete the peer's WebSocket close handshake, never initiate one ourselves.
                if (!data.isEmpty() && (quint8(data[0]) & 0x0f) == 8) {
                    socket->write(QByteArray::fromHex("8800"));
                    socket->disconnectFromHost();
                }
                return;
            }
            auto request = socket->property("request").toByteArray() + data;
            socket->setProperty("request", request);
            if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
            socket->setProperty("handled", true);
            if (request.startsWith("GET /socket ")) {
                QByteArray clientKey;
                for (auto line : request.split('\n'))
                    if (line.toLower().startsWith("sec-websocket-key:")) clientKey = line.mid(line.indexOf(':')+1).trimmed();
                if (clientKey.isEmpty()) { socket->abort(); return; }
                ++handshakes;
                activeSockets.insert(socket);
                socket->setProperty("upgraded", true);
                const auto accept = QCryptographicHash::hash(clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", QCryptographicHash::Sha1).toBase64();
                socket->write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + accept + "\r\n\r\n");
                socket->write(QByteArray::fromHex("8104") + "live");
                return;
            }
            const QByteArray body("<!doctype html><title>TLS fixture</title>");
            socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
            socket->disconnectFromHost();
        });
        socket->startServerEncryption();
    }
};

inline bool runTlsBoundaryTests() {
    if (!QSslSocket::supportsSsl()) { qCritical() << "TLS_BOUNDARY_TEST FAIL: no TLS backend"; return false; }
    QFile certFile("tests/security/fixtures/tls/localhost-cert.pem"), keyFile("tests/security/fixtures/tls/localhost-key.pem");
    if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) return false;
    BoundaryTlsServer tls;
    tls.certificate = QSslCertificate(certFile.readAll());
    tls.key = QSslKey(keyFile.readAll(), QSsl::Rsa);
    if (tls.certificate.isNull() || tls.key.isNull() || !tls.listen(QHostAddress::LocalHost)) return false;
    QTcpServer source;
    if (!source.listen(QHostAddress::LocalHost)) return false;
    QObject::connect(&source, &QTcpServer::newConnection, &source, [&] {
        while (auto socket = source.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                socket->readAll();
                const QByteArray body("<!doctype html><title>Local source</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    const QUrl secure(QString("https://127.0.0.1:%1/page").arg(tls.serverPort()));
    const auto wsUrl = QString("wss://127.0.0.1:%1/socket").arg(tls.serverPort());
    QWebEngineProfile negativeProfile, positiveProfile;
    RequestObserver negativeObserver(&negativeProfile), positiveObserver(&positiveProfile);
    negativeObserver.setHttpsOnly(false); // Test-only loopback source page, not a certificate exception.
    negativeProfile.setUrlRequestInterceptor(&negativeObserver);
    positiveProfile.setUrlRequestInterceptor(&positiveObserver);
    QWebEnginePage negative(&negativeProfile);
    auto positive = std::make_unique<QWebEnginePage>(&positiveProfile);
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    int phase = 0, rejected = 0, explicitlyAccepted = 0;
    bool passed = false;
    auto fail = [&](const QString &why) { qCritical() << "TLS_BOUNDARY_TEST FAIL" << phase << why; loop.quit(); };
    QObject::connect(&deadline, &QTimer::timeout, &loop, [&] { fail("timeout"); });
    QObject::connect(&negative, &QWebEnginePage::certificateError, &loop, [&](QWebEngineCertificateError error) {
        ++rejected; error.rejectCertificate();
    });
    QObject::connect(positive.get(), &QWebEnginePage::certificateError, &loop, [&](QWebEngineCertificateError error) {
        // Positive fixture ONLY: pin this leaf and exact loopback endpoint in an isolated test profile.
        const auto chain = error.certificateChain();
        if (error.url().host() == "127.0.0.1" && error.url().port() == tls.serverPort()
            && !chain.isEmpty() && chain.first().digest(QCryptographicHash::Sha256) == tls.certificate.digest(QCryptographicHash::Sha256)) {
            ++explicitlyAccepted; error.acceptCertificate();
        } else { error.rejectCertificate(); }
    });
    auto negativeWs = [&](const QString &label) {
        negative.runJavaScript(QString(R"JS((()=>{let s=new WebSocket('%1');s.onopen=()=>{s.close();document.title='%2-open'};s.onerror=()=>document.title='%2-blocked'})())JS").arg(wsUrl,label));
    };
    QObject::connect(&negative, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
        if (phase == 0) {
            if (ok || rejected == 0 || tls.handshakes != 0) { fail("untrusted HTTPS certificate was not rejected"); return; }
            phase = 1;
            QTimer::singleShot(0, &negative, [&] { negative.load(QUrl(QString("http://127.0.0.1:%1/page").arg(source.serverPort()))); });
        } else if (phase == 1) {
            if (!ok) { fail("local source failed"); return; }
            phase = 2; negativeWs("untrusted");
        }
    });
    QObject::connect(&negative, &QWebEnginePage::titleChanged, &loop, [&](const QString &title) {
        if (phase == 2 && title.startsWith("untrusted-")) {
            if (title != "untrusted-blocked" || tls.handshakes != 0) { fail("untrusted WSS escaped"); return; }
            phase = 3; positive->load(secure);
        } else if (phase == 5 && title.startsWith("after-")) {
            if (title != "after-blocked" || tls.handshakes != 1 || tls.activeSockets.size() != 1) { fail("certificate allowance leaked or live control missing"); return; }
            phase = 6;
            QTimer::singleShot(0, &loop, [&] { positive.reset(); });
        }
    });
    QObject::connect(positive.get(), &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
        if (phase != 3) return;
        if (!ok || explicitlyAccepted == 0) { fail("pinned positive TLS fixture failed"); return; }
        phase = 4;
        positive->runJavaScript(QString(R"JS((()=>{window.liveSocket=new WebSocket('%1');liveSocket.onmessage=e=>{if(e.data==='live')document.title='wss-live'};liveSocket.onerror=()=>document.title='wss-failed'})())JS").arg(wsUrl));
    });
    QObject::connect(positive.get(), &QWebEnginePage::titleChanged, &loop, [&](const QString &title) {
        if (phase != 4) return;
        if (title == "wss-failed") { fail("positive WSS control failed"); return; }
        if (title == "wss-live") { phase = 5; negativeWs("after"); }
    });
    tls.onSocketClosed = [&] {
        if (phase != 6) return;
        if (tls.closedSockets != 1 || !tls.activeSockets.isEmpty()) { fail("socket teardown counters"); return; }
        qInfo() << "TLS_BOUNDARY_TEST PASS: untrusted HTTPS/WSS rejected; exact pinned test-only positive WSS message; isolated certificate exception; destroying owning page closed active WSS";
        passed = true; loop.quit();
    };
    deadline.start(25000);
    negative.load(secure);
    loop.exec();
    deadline.stop();
    tls.onSocketClosed = {};
    return passed;
}
