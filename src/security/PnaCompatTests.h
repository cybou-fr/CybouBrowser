#pragma once
#include <QEventLoop>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include <QtWebEngineCore/QWebEngineProfileBuilder>
#include <QFile>
#include "HttpsPnaProbe.h"

// Compatibility guard for applyEngineHardening(): only the public->local combination may be
// blocked. Engine decision only (no app interceptor). Address spaces are assigned by
// --ip-address-space-overrides (set in tests/test_main.cpp) on fixed ports.
inline constexpr quint16 kPnaPublicSource = 38471, kPnaLocalTarget = 38472, kPnaLocalSource = 38473, kPnaPublicTarget = 38474;
inline constexpr quint16 kPnaSecurePublicSource = 38476, kPnaSecureAllowTarget = 38477, kPnaSecureDenyTarget = 38478, kPnaSecureLocalSource = 38479;

inline int runPnaCompatTest() {
    QMap<QByteArray, int> hits;
    QList<QTcpServer *> servers;
    for (quint16 port : {kPnaPublicSource, kPnaLocalTarget, kPnaLocalSource, kPnaPublicTarget}) {
        auto *server = new QTcpServer;
        servers << server;
        if (!server->listen(QHostAddress::LocalHost, port)) { qCritical() << "PNA_COMPAT FAIL: port busy" << port; return 2; }
        QObject::connect(server, &QTcpServer::newConnection, server, [server, &hits, port] {
            while (auto socket = server->nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [&hits, socket, port] {
                    const auto data = socket->property("headers").toByteArray() + socket->readAll();
                    socket->setProperty("headers", data);
                    if (socket->property("handled").toBool() || !data.contains("\r\n\r\n")) return;
                    socket->setProperty("handled", true);
                    const auto parts = data.left(data.indexOf("\r\n")).split(' ');
                    if (parts.size() < 2) { socket->abort(); return; }
                    ++hits[QByteArray::number(port) + parts[1]];
                    const QByteArray body("<!doctype html><title>Compat ready</title>");
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-store\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }
    QWebEngineProfile profile; // engine decision only
    QWebEnginePage page(&profile);
    auto run = [&](const QString &sourceUrl, const QString &script) {
        QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QString title; bool loaded = false;
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        auto c1 = QObject::connect(&page, &QWebEnginePage::titleChanged, &loop, [&](const QString &t) { if (t == "done") { title = t; loop.quit(); } });
        auto c2 = QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
            loaded = ok;
            if (!ok) loop.quit(); else page.runJavaScript(script);
        });
        timeout.start(20000); page.load(QUrl(sourceUrl)); loop.exec();
        QObject::disconnect(c1); QObject::disconnect(c2);
        if (!loaded || title != "done") return QString("LOAD_FAIL");
        QString result; QEventLoop get;
        page.runJavaScript("window.__r", [&](const QVariant &v) { result = v.toString(); get.quit(); });
        QTimer::singleShot(3000, &get, &QEventLoop::quit); get.exec();
        return result;
    };
    auto fetchScript = [](const QString &url) {
        return QString("(async()=>{try{await fetch('%1',{mode:'no-cors',cache:'no-store'});window.__r='ok'}catch(e){window.__r='blocked'}document.title='done'})()").arg(url);
    };
    struct Case { const char *name; quint16 source; QString target; QString expect; QByteArray hitKey; int hitCount; };
    const auto host = [](quint16 p, const char *name) { return QString("http://%1:%2").arg(name).arg(p); };
    const QList<Case> cases = {
        {"public->local literal", kPnaPublicSource, QString("http://127.0.0.1:%1/a").arg(kPnaLocalTarget), "blocked", QByteArray::number(kPnaLocalTarget) + "/a", 0},
        {"public->local alias", kPnaPublicSource, host(kPnaLocalTarget, "cybou-alias.test") + "/b", "blocked", QByteArray::number(kPnaLocalTarget) + "/b", 0},
        {"public->public", kPnaPublicSource, host(kPnaPublicTarget, "cybou-alias.test") + "/c", "ok", QByteArray::number(kPnaPublicTarget) + "/c", 1},
        {"local->local literal", kPnaLocalSource, QString("http://127.0.0.1:%1/d").arg(kPnaLocalTarget), "ok", QByteArray::number(kPnaLocalTarget) + "/d", 1},
        {"local->local alias", kPnaLocalSource, host(kPnaLocalTarget, "cybou-alias.test") + "/e", "ok", QByteArray::number(kPnaLocalTarget) + "/e", 1},
        {"local->public", kPnaLocalSource, host(kPnaPublicTarget, "cybou-alias.test") + "/f", "ok", QByteArray::number(kPnaPublicTarget) + "/f", 1},
    };
    for (const auto &c : cases) {
        const auto result = run(host(c.source, "cybou-source.test") + "/source", fetchScript(c.target));
        if (result != c.expect || hits.value(c.hitKey) != c.hitCount) {
            qCritical() << "PNA_COMPAT FAIL:" << c.name << "result" << result << "expected" << c.expect << "hits" << hits.value(c.hitKey) << "expected" << c.hitCount;
            return 1;
        }
        qInfo() << "PNA_COMPAT_CASE PASS:" << c.name << result;
    }
    // Secure (HTTPS) public source: reaches a local target only if it answers the PNA preflight.
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    {
        QFile rootFile("tests/security/fixtures/tls/test-ca-cert.pem"), leafFile("tests/security/fixtures/tls/pna-cert.pem"), keyFile("tests/security/fixtures/tls/localhost-key.pem");
        if (!rootFile.open(QIODevice::ReadOnly) || !leafFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) { qCritical() << "PNA_COMPAT FAIL: TLS fixtures"; return 2; }
        const QSslCertificate root(rootFile.readAll()), leaf(leafFile.readAll());
        const QSslKey key(keyFile.readAll(), QSsl::Rsa);
        ProbeTlsServer secureSource, allowTarget, denyTarget, secureLocalSource;
        for (auto *s : {&secureSource, &allowTarget, &denyTarget, &secureLocalSource}) { s->certificate = leaf; s->key = key; s->hits = &hits; }
        denyTarget.allowPrivateNetwork = false;
        if (!secureSource.listen(QHostAddress::LocalHost, kPnaSecurePublicSource) || !allowTarget.listen(QHostAddress::LocalHost, kPnaSecureAllowTarget)
            || !denyTarget.listen(QHostAddress::LocalHost, kPnaSecureDenyTarget) || !secureLocalSource.listen(QHostAddress::LocalHost, kPnaSecureLocalSource)) { qCritical() << "PNA_COMPAT FAIL: secure ports busy"; return 2; }
        QWebEngineProfileBuilder builder; builder.setAdditionalTrustedCertificates({root});
        std::unique_ptr<QWebEngineProfile> secureProfile(builder.createProfile(QString()));
        QWebEnginePage securePage(secureProfile.get());
        QObject::connect(&securePage, &QWebEnginePage::certificateError, &securePage, [](QWebEngineCertificateError e) { e.rejectCertificate(); });
        auto runSecure = [&](quint16 sourcePort, const QString &target) {
            QEventLoop loop; QTimer timeout; timeout.setSingleShot(true); bool done = false, loaded = false;
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            auto c1 = QObject::connect(&securePage, &QWebEnginePage::titleChanged, &loop, [&](const QString &t) { if (t == "done") { done = true; loop.quit(); } });
            auto c2 = QObject::connect(&securePage, &QWebEnginePage::loadFinished, &loop, [&](bool ok) { loaded = ok; if (!ok) loop.quit(); else securePage.runJavaScript(fetchScript(target)); });
            timeout.start(20000); securePage.load(QUrl(QString("https://cybou-source.test:%1/source").arg(sourcePort))); loop.exec();
            QObject::disconnect(c1); QObject::disconnect(c2);
            if (!loaded || !done) return QString("LOAD_FAIL");
            QString result; QEventLoop get;
            securePage.runJavaScript("window.__r", [&](const QVariant &v) { result = v.toString(); get.quit(); });
            QTimer::singleShot(3000, &get, &QEventLoop::quit); get.exec();
            return result;
        };
        struct Secure { const char *name; quint16 source; QString target; QString expect; QByteArray getKey; int getCount; QByteArray optionsKey; int optionsCount; };
        const auto https = [](const char *name, quint16 p) { return QString("https://%1:%2").arg(name).arg(p); };
        const QList<Secure> secureCases = {
            {"https public->local preflight allowed", kPnaSecurePublicSource, https("127.0.0.1", kPnaSecureAllowTarget) + "/g", "ok", QByteArray::number(kPnaSecureAllowTarget) + " GET /g", 1, QByteArray::number(kPnaSecureAllowTarget) + " OPTIONS /g", 1},
            {"https public->local preflight denied", kPnaSecurePublicSource, https("127.0.0.1", kPnaSecureDenyTarget) + "/h", "blocked", QByteArray::number(kPnaSecureDenyTarget) + " GET /h", 0, QByteArray::number(kPnaSecureDenyTarget) + " OPTIONS /h", 1},
            {"https public->local alias preflight denied", kPnaSecurePublicSource, https("cybou-alias.test", kPnaSecureDenyTarget) + "/i", "blocked", QByteArray::number(kPnaSecureDenyTarget) + " GET /i", 0, QByteArray::number(kPnaSecureDenyTarget) + " OPTIONS /i", 1},
            {"https local->local needs no preflight", kPnaSecureLocalSource, https("127.0.0.1", kPnaSecureDenyTarget) + "/j", "ok", QByteArray::number(kPnaSecureDenyTarget) + " GET /j", 1, QByteArray::number(kPnaSecureDenyTarget) + " OPTIONS /j", 0},
        };
        for (const auto &c : secureCases) {
            const auto result = runSecure(c.source, c.target);
            if (result != c.expect || hits.value(c.getKey) != c.getCount || hits.value(c.optionsKey) != c.optionsCount) {
                qCritical() << "PNA_COMPAT FAIL:" << c.name << "result" << result << "GET" << hits.value(c.getKey) << "OPTIONS" << hits.value(c.optionsKey);
                return 1;
            }
            qInfo() << "PNA_COMPAT_CASE PASS:" << c.name << result;
        }
    }
#endif
    // Direct user navigation to a local address is never a PNA request.
    QEventLoop direct; QTimer::singleShot(5000, &direct, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::loadFinished, &direct, &QEventLoop::quit);
    page.load(QUrl(QString("http://127.0.0.1:%1/direct").arg(kPnaLocalTarget)));
    direct.exec();
    if (hits.value(QByteArray::number(kPnaLocalTarget) + "/direct") != 1) { qCritical() << "PNA_COMPAT FAIL: direct local navigation"; return 1; }
    qInfo() << "PNA_COMPAT_CASE PASS: direct local navigation";
    qInfo() << "PNA_COMPAT_TEST PASS: insecure public->local blocked; secure public->local needs a PNA preflight grant; local->local, public->public, local->public and direct navigation preserved";
    qDeleteAll(servers);
    return 0;
}
