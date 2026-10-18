#pragma once
#include <QEventLoop>
#include <QFile>
#include <QMap>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include <QtWebEngineCore/QWebEngineProfileBuilder>

// Diagnostic: does the raw engine block an HTTPS (secure-context) public-space page from reaching
// local targets? Source port 38471 is overridden to public via --ip-address-space-overrides. Extra
// Chromium features are supplied through QTWEBENGINE_CHROMIUM_FLAGS; no app interceptor is installed.
// Prints HTTPS_PNA_RESULT; exit 0 means the experiment ran, not that anything is enforced.
class ProbeTlsServer final : public QTcpServer {
public:
    QSslCertificate certificate;
    QSslKey key;
    bool tls = true;
    bool allowPrivateNetwork = true;
    QMap<QByteArray, int> *hits = nullptr;
protected:
    void incomingConnection(qintptr descriptor) override {
        auto socket = new QSslSocket(this);
        if (tls) { socket->setLocalCertificate(certificate); socket->setPrivateKey(key); socket->setPeerVerifyMode(QSslSocket::VerifyNone); }
        if (!socket->setSocketDescriptor(descriptor)) { delete socket; return; }
        connect(socket, &QSslSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QSslSocket::readyRead, socket, [this, socket] {
            const auto request = socket->property("request").toByteArray() + socket->readAll();
            socket->setProperty("request", request);
            if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
            socket->setProperty("handled", true);
            const auto parts = request.left(request.indexOf("\r\n")).split(' ');
            if (parts.size() < 2) { socket->abort(); return; }
            ++(*hits)[QByteArray::number(serverPort()) + " " + parts[0] + " " + parts[1]];
            if (parts[0] == "OPTIONS") {
                socket->write(QByteArray("HTTP/1.1 204 No Content\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET\r\n")
                              + (allowPrivateNetwork ? "Access-Control-Allow-Private-Network: true\r\n" : "") + "Content-Length: 0\r\n\r\n");
            } else {
                const QByteArray body("<!doctype html><title>Probe ready</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-store\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
            }
            socket->disconnectFromHost();
        });
        if (tls) socket->startServerEncryption();
    }
};

inline int runHttpsPnaProbe() {
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
    qCritical() << "HTTPS_PNA_PROBE BLOCKED: profile-local CA requires Qt 6.10+"; return 2;
#else
    QFile rootFile("tests/security/fixtures/tls/test-ca-cert.pem"), leafFile("tests/security/fixtures/tls/pna-cert.pem"), keyFile("tests/security/fixtures/tls/localhost-key.pem");
    if (!rootFile.open(QIODevice::ReadOnly) || !leafFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) { qCritical() << "HTTPS_PNA_PROBE FAIL: fixtures (run from repo root)"; return 2; }
    const QSslCertificate root(rootFile.readAll()), leaf(leafFile.readAll());
    const QSslKey key(keyFile.readAll(), QSsl::Rsa);
    if (root.isNull() || leaf.isNull() || key.isNull()) return 2;
    QMap<QByteArray, int> hits;
    ProbeTlsServer source, localTls, localHttp;
    for (auto *server : {&source, &localTls}) { server->certificate = leaf; server->key = key; }
    localHttp.tls = false;
    if (qEnvironmentVariableIsSet("CYBOU_PROBE_DENY_PNA")) for (auto *server : {&localTls, &localHttp}) server->allowPrivateNetwork = false; // preflight lacks Allow-Private-Network
    for (auto *server : {&source, &localTls, &localHttp}) server->hits = &hits;
    if (!source.listen(QHostAddress::LocalHost, 38471) || !localTls.listen(QHostAddress::LocalHost, 38472) || !localHttp.listen(QHostAddress::LocalHost, 38475)) {
        qCritical() << "HTTPS_PNA_PROBE FAIL: ports busy"; return 2;
    }
    QWebEngineProfileBuilder builder; builder.setAdditionalTrustedCertificates({root});
    std::unique_ptr<QWebEngineProfile> profile(builder.createProfile(QString()));
    QWebEnginePage page(profile.get());
    QObject::connect(&page, &QWebEnginePage::certificateError, &page, [](QWebEngineCertificateError error) { error.rejectCertificate(); });
    QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::titleChanged, &loop, [&](const QString &t) { if (t == "done") loop.quit(); });
    QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
        if (!ok) { qCritical() << "HTTPS_PNA_PROBE FAIL: https source load"; loop.quit(); return; }
        page.runJavaScript(QStringLiteral(R"JS((async()=>{
            const targets={httpsLiteral:'https://127.0.0.1:38472/a',httpsAlias:'https://cybou-alias.test:38472/b',httpLiteral:'http://127.0.0.1:38475/c'};
            const out={secure:window.isSecureContext};
            for(const [name,url] of Object.entries(targets)) for(const mode of ['no-cors','cors']){
                try{await fetch(url,{mode,cache:'no-store'});out[name+'/'+mode]='ok'}catch(e){out[name+'/'+mode]='blocked'}
            }
            window.__r=JSON.stringify(out);document.title='done';
        })())JS"));
    });
    timeout.start(30000);
    page.load(QUrl("https://cybou-source.test:38471/source"));
    loop.exec();
    QString result; QEventLoop get;
    page.runJavaScript("window.__r", [&](const QVariant &v) { result = v.toString(); get.quit(); });
    QTimer::singleShot(3000, &get, &QEventLoop::quit); get.exec();
    qInfo().noquote() << "HTTPS_PNA_RESULT" << result;
    QStringList lines; for (auto it = hits.cbegin(); it != hits.cend(); ++it) lines << QString("%1 x%2").arg(QString(it.key())).arg(it.value());
    qInfo().noquote() << "HTTPS_PNA_HITS" << lines.join(" | ");
    return result.isEmpty() ? 2 : 0;
#endif
}
