#pragma once
#include <QEventLoop>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "../controlbar/RequestObserver.h"
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
                    if (parts[1].startsWith("/redir-")) {
                        const QByteArray location = parts[1].startsWith("/redir-literal") ? "http://127.0.0.1:38472/k" : "http://cybou-alias.test:38472/l";
                        socket->write("HTTP/1.1 302 Found\r\nConnection: close\r\nLocation: " + location + "\r\nContent-Length: 0\r\n\r\n");
                        socket->disconnectFromHost();
                        return;
                    }
                    const QByteArray body("<!doctype html><title>Compat ready</title>");
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-store\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }
    QWebEngineProfile profile; // engine decision only
    QWebEnginePage page(&profile);
    QWebEnginePage *current = &page; // page the run() helper drives
    auto run = [&](const QString &sourceUrl, const QString &script) {
        QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QString title; bool loaded = false;
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        auto c1 = QObject::connect(current, &QWebEnginePage::titleChanged, &loop, [&](const QString &t) { if (t == "done") { title = t; loop.quit(); } });
        auto c2 = QObject::connect(current, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
            loaded = ok;
            if (!ok) loop.quit(); else current->runJavaScript(script);
        });
        timeout.start(20000); current->load(QUrl(sourceUrl)); loop.exec();
        QObject::disconnect(c1); QObject::disconnect(c2);
        if (!loaded || title != "done") return QString("LOAD_FAIL");
        QString result; QEventLoop get;
        current->runJavaScript("window.__r", [&](const QVariant &v) { result = v.toString(); get.quit(); });
        QTimer::singleShot(3000, &get, &QEventLoop::quit); get.exec();
        return result;
    };
    auto fetchScript = [](const QString &url) {
        return QString("(async()=>{try{await fetch('%1',{mode:'no-cors',cache:'no-store'});window.__r='ok'}catch(e){window.__r='blocked'}document.title='done'})()").arg(url);
    };
    // "ws:" targets open a WebSocket. The plain fixture never completes the handshake, so the
    // outcome is judged by target hit counters (a blocked socket never connects), not by events.
    auto scriptFor = [&](const QString &target) {
        if (target.startsWith("worker:")) {
            // Dedicated worker (blob URL) performs the fetch; it inherits the creator's address space.
            return QString("(()=>{const src=\"fetch('%1',{mode:'no-cors',cache:'no-store'}).then(()=>postMessage('ok'),()=>postMessage('blocked'))\";"
                           "const w=new Worker(URL.createObjectURL(new Blob([src])));w.onmessage=e=>{window.__r=e.data;document.title='done'};"
                           "w.onerror=()=>{window.__r='worker-error';document.title='done'};setTimeout(()=>{if(!window.__r){window.__r='timeout';document.title='done'}},12000)})()").arg(target.mid(7));
        }
        if (!target.startsWith("ws:")) return fetchScript(target);
        return QString("(()=>{const done=()=>{window.__r='settled';document.title='done'};const w=new WebSocket('%1');w.onerror=done;w.onclose=done;w.onopen=done;setTimeout(done,8000)})()").arg(target.mid(3));
    };
    struct Case { const char *name; quint16 source; QString target; QString expect; QByteArray hitKey; int hitCount; bool knownGap = false; };
    const auto host = [](quint16 p, const char *name) { return QString("http://%1:%2").arg(name).arg(p); };
    const QList<Case> cases = {
        {"public->local literal", kPnaPublicSource, QString("http://127.0.0.1:%1/a").arg(kPnaLocalTarget), "blocked", QByteArray::number(kPnaLocalTarget) + "/a", 0},
        {"public->local alias", kPnaPublicSource, host(kPnaLocalTarget, "cybou-alias.test") + "/b", "blocked", QByteArray::number(kPnaLocalTarget) + "/b", 0},
        {"public->public", kPnaPublicSource, host(kPnaPublicTarget, "cybou-alias.test") + "/c", "ok", QByteArray::number(kPnaPublicTarget) + "/c", 1},
        {"local->local literal", kPnaLocalSource, QString("http://127.0.0.1:%1/d").arg(kPnaLocalTarget), "ok", QByteArray::number(kPnaLocalTarget) + "/d", 1},
        {"local->local alias", kPnaLocalSource, host(kPnaLocalTarget, "cybou-alias.test") + "/e", "ok", QByteArray::number(kPnaLocalTarget) + "/e", 1},
        {"public->public->local redirect literal", kPnaPublicSource, host(kPnaPublicTarget, "cybou-alias.test") + "/redir-literal", "*", QByteArray::number(kPnaLocalTarget) + "/k", 0},
        {"public->public->local redirect alias", kPnaPublicSource, host(kPnaPublicTarget, "cybou-alias.test") + "/redir-alias", "*", QByteArray::number(kPnaLocalTarget) + "/l", 0},
        {"local->public->local redirect", kPnaLocalSource, host(kPnaPublicTarget, "cybou-alias.test") + "/redir-literal2", "*", QByteArray::number(kPnaLocalTarget) + "/k", 1},
        {"public ws->local literal", kPnaPublicSource, QString("ws:ws://127.0.0.1:%1/m").arg(kPnaLocalTarget), "*", QByteArray::number(kPnaLocalTarget) + "/m", 0, true},
        {"public ws->local alias", kPnaPublicSource, QString("ws:ws://cybou-alias.test:%1/n").arg(kPnaLocalTarget), "*", QByteArray::number(kPnaLocalTarget) + "/n", 0, true},
        {"local ws->local", kPnaLocalSource, QString("ws:ws://127.0.0.1:%1/o").arg(kPnaLocalTarget), "*", QByteArray::number(kPnaLocalTarget) + "/o", 1},
        {"public worker->local literal", kPnaPublicSource, QString("worker:http://127.0.0.1:%1/s").arg(kPnaLocalTarget), "blocked", QByteArray::number(kPnaLocalTarget) + "/s", 0},
        {"public worker->local alias", kPnaPublicSource, QString("worker:http://cybou-alias.test:%1/t").arg(kPnaLocalTarget), "blocked", QByteArray::number(kPnaLocalTarget) + "/t", 0},
        {"local worker->local", kPnaLocalSource, QString("worker:http://127.0.0.1:%1/u").arg(kPnaLocalTarget), "ok", QByteArray::number(kPnaLocalTarget) + "/u", 1},
        {"local->public", kPnaLocalSource, host(kPnaPublicTarget, "cybou-alias.test") + "/f", "ok", QByteArray::number(kPnaPublicTarget) + "/f", 1},
    };
    int knownGaps = 0;
    for (const auto &c : cases) {
        const auto result = run(host(c.source, "cybou-source.test") + "/source", scriptFor(c.target));
        if (c.knownGap && hits.value(c.hitKey) != c.hitCount) {
            ++knownGaps;
            qWarning() << "PNA_COMPAT_KNOWN_GAP:" << c.name << "reached target" << hits.value(c.hitKey) << "time(s); engine PNA does not cover WebSocket";
            continue;
        }
        if ((c.expect != "*" && result != c.expect) || hits.value(c.hitKey) != c.hitCount) {
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
    // Application layer: RequestObserver on a public-source page. Source loads over HTTP only so
    // the WebSocket policy, not strict-HTTPS denial of the source, decides the outcome.
    {
        QWebEngineProfile appProfile;
        RequestObserver observer(&appProfile);
        observer.setHttpsOnly(false);
        // Stub resolver: the engine resolves test hostnames with --host-resolver-rules, which the OS
        // resolver cannot see. The alias is answered as loopback; the other name as a public address.
        observer.setHostResolver([](const QString &host) {
            if (host == "cybou-alias.test") return QList<QHostAddress>{QHostAddress(QHostAddress::LocalHost)};
            return QList<QHostAddress>{QHostAddress("203.0.113.9")};
        });
        appProfile.setUrlRequestInterceptor(&observer);
        QWebEnginePage appPage(&appProfile);
        current = &appPage;
        const auto before = knownGaps;
        const struct { const char *name; QString target; QByteArray hitKey; bool knownGap; } appCases[] = {
            {"app interceptor public ws->local literal", QString("ws:ws://127.0.0.1:%1/p").arg(kPnaLocalTarget), QByteArray::number(kPnaLocalTarget) + "/p", false},
            {"app interceptor public ws->local alias", QString("ws:ws://cybou-alias.test:%1/q").arg(kPnaLocalTarget), QByteArray::number(kPnaLocalTarget) + "/q", false},
        };
        for (const auto &c : appCases) {
            const auto result = run(host(kPnaPublicSource, "cybou-source.test") + "/source", scriptFor(c.target));
            const int reached = hits.value(c.hitKey);
            if (reached != 0 && c.knownGap) {
                ++knownGaps;
                qWarning() << "PNA_COMPAT_KNOWN_GAP:" << c.name << "reached target" << reached << "time(s); URL classification cannot see the resolved address";
            } else if (reached != 0 || result == "LOAD_FAIL") {
                qCritical() << "PNA_COMPAT FAIL:" << c.name << "result" << result << "target hits" << reached;
                return 1;
            } else {
                qInfo() << "PNA_COMPAT_CASE PASS:" << c.name << "blocked before the target";
            }
        }
        Q_UNUSED(before);
        // Control: a WebSocket whose name resolves to a public address is not blocked.
        run(host(kPnaPublicSource, "cybou-source.test") + "/source", scriptFor(QString("ws:ws://cybou-source.test:%1/r").arg(kPnaPublicTarget)));
        if (hits.value(QByteArray::number(kPnaPublicTarget) + "/r") != 1) { qCritical() << "PNA_COMPAT FAIL: public-resolving ws wrongly blocked"; return 1; }
        qInfo() << "PNA_COMPAT_CASE PASS: app interceptor public ws->public name allowed";
        current = &page;
    }
    // Direct user navigation to a local address is never a PNA request.
    QEventLoop direct; QTimer::singleShot(5000, &direct, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::loadFinished, &direct, &QEventLoop::quit);
    page.load(QUrl(QString("http://127.0.0.1:%1/direct").arg(kPnaLocalTarget)));
    direct.exec();
    if (hits.value(QByteArray::number(kPnaLocalTarget) + "/direct") != 1) { qCritical() << "PNA_COMPAT FAIL: direct local navigation"; return 1; }
    qInfo() << "PNA_COMPAT_CASE PASS: direct local navigation";
    qInfo() << "PNA_COMPAT_TEST PASS (known gaps:" << knownGaps << "- engine PNA does not cover WebSocket; app resolver is best-effort): insecure public->local blocked; secure public->local needs a PNA preflight grant; local->local, public->public, local->public and direct navigation preserved";
    qDeleteAll(servers);
    return 0;
}
