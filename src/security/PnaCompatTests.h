#pragma once
#include <QEventLoop>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>

// Compatibility guard for applyEngineHardening(): only the public->local combination may be
// blocked. Engine decision only (no app interceptor). Address spaces are assigned by
// --ip-address-space-overrides (set in tests/test_main.cpp) on fixed ports.
inline constexpr quint16 kPnaPublicSource = 38471, kPnaLocalTarget = 38472, kPnaLocalSource = 38473, kPnaPublicTarget = 38474;

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
    // Direct user navigation to a local address is never a PNA request.
    QEventLoop direct; QTimer::singleShot(5000, &direct, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::loadFinished, &direct, &QEventLoop::quit);
    page.load(QUrl(QString("http://127.0.0.1:%1/direct").arg(kPnaLocalTarget)));
    direct.exec();
    if (hits.value(QByteArray::number(kPnaLocalTarget) + "/direct") != 1) { qCritical() << "PNA_COMPAT FAIL: direct local navigation"; return 1; }
    qInfo() << "PNA_COMPAT_CASE PASS: direct local navigation";
    qInfo() << "PNA_COMPAT_TEST PASS: only public->local blocked; local->local, public->public, local->public and direct navigation preserved";
    qDeleteAll(servers);
    return 0;
}
