#pragma once
#include <QEventLoop>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "../controlbar/RequestObserver.h"

// Diagnostic only: resolver rules map both ordinary hostnames to loopback.
// This is not a public-address source, a CNAME chain, or changing DNS answers.
inline int runPrivateNetworkProbe() {
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QMap<QByteArray, int> hits;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                const auto data = socket->property("headers").toByteArray() + socket->readAll();
                socket->setProperty("headers", data);
                if (socket->property("handled").toBool() || !data.contains("\r\n\r\n")) return;
                socket->setProperty("handled", true);
                const auto parts = data.left(data.indexOf("\r\n")).split(' ');
                if (parts.size() < 2) { socket->abort(); return; }
                ++hits[parts[1]];
                const QByteArray body("<!doctype html><title>Probe ready</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-store\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    const auto base = QString("http://127.0.0.1:%1").arg(server.serverPort());
    const auto source = QString("http://cybou-source.test:%1/source").arg(server.serverPort());
    const auto alias = QString("http://cybou-alias.test:%1/alias").arg(server.serverPort());
    QWebEngineProfile profile;
    RequestObserver observer(&profile);
    observer.setHttpsOnly(false); // Separate PNA from the default HTTP deny.
    profile.setUrlRequestInterceptor(&observer);
    QWebEnginePage page(&profile);
    auto load = [&](const QUrl &url) {
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        bool finished = false, success = false;
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) { finished = true; success = ok; loop.quit(); });
        timeout.start(10000); page.load(url); loop.exec();
        if (!finished) page.triggerAction(QWebEnginePage::Stop);
        return finished && success;
    };
    // Both endpoint forms must work as explicit direct user navigations.
    if (!load(QUrl(base + "/literal-control")) || !load(QUrl(alias + "-control")) || !load(QUrl(source))
        || hits.value("/literal-control") != 1 || hits.value("/alias-control") != 1 || hits.value("/source") != 1) {
        qCritical() << "PRIVATE_NETWORK_PROBE FAIL: direct endpoint/source controls" << hits; return 2;
    }
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    bool completed = false;
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::titleChanged, &loop, [&](const QString &title) {
        if (title == "Probe complete") { completed = true; loop.quit(); }
    });
    page.runJavaScript(QString(R"JS((async()=>{
        const targets=['http://127.0.0.1:%1/literal','http://localhost:%1/local-name',
            'http://2130706433:%1/decimal','http://0x7f000001:%1/hex','%2'];
        for(const target of targets){try{await fetch(target,{mode:'no-cors',cache:'no-store'})}catch(e){}}
        document.title='Probe complete';
    })())JS").arg(server.serverPort()).arg(alias));
    timeout.start(15000); loop.exec();
    if (!completed) { page.triggerAction(QWebEnginePage::Stop); qCritical() << "PRIVATE_NETWORK_PROBE FAIL: fetch timeout"; return 2; }
    int denied = 0;
    bool aliasObserved = false;
    for (const auto &value : observer.sample()) {
        const auto event = value.toMap();
        if (event.value("firstParty").toString().contains("cybou-source.test") && event.value("reason") == "private-network-access" && event.value("blocked").toBool()) ++denied;
        if (event.value("firstParty").toString().contains("cybou-source.test") && event.value("origin").toString().contains("cybou-alias.test")) aliasObserved = true;
    }
    for (const auto &path : {"/literal", "/local-name", "/decimal", "/hex"}) {
        if (hits.value(path) != 0) { qCritical() << "PRIVATE_NETWORK_PROBE FAIL: literal request reached server" << path; return 2; }
    }
    if (denied != 4 || !aliasObserved || hits.value("/alias") > 1) {
        qCritical() << "PRIVATE_NETWORK_PROBE FAIL: missing deny/alias observations" << denied << aliasObserved << hits; return 2;
    }
    qInfo() << "PRIVATE_NETWORK_LITERAL_TEST PASS: four actual fetches denied with native metadata and zero target hits; direct controls succeeded";
    if (hits.value("/alias") == 1) {
        qWarning() << "PRIVATE_NETWORK_ALIAS_GAP: URL-public hostname resolved to loopback reached endpoint; no runtime DNS enforcement demonstrated";
        return 3; // Diagnostic gap, never a release-boundary PASS.
    }
    qWarning() << "PRIVATE_NETWORK_ALIAS_INCONCLUSIVE: alias fetch did not reach endpoint; requires engine reason and true public/private fixture";
    return 4;
}

// Engine-only experiment (no app interceptor): which loopback fetches does Chromium itself
// block when the page's server is overridden to the public address space? Fixed ports are
// required because --ip-address-space-overrides is set before engine initialisation.
inline constexpr quint16 kEngineProbeSourcePort = 38471;
inline constexpr quint16 kEngineProbeTargetPort = 38472;

inline int runEnginePnaProbe() {
    QMap<QByteArray, int> hits;
    auto serve = [&](QTcpServer &server, quint16 port) {
        if (!server.listen(QHostAddress::LocalHost, port)) return false;
        QObject::connect(&server, &QTcpServer::newConnection, &server, [&server, &hits] {
            while (auto socket = server.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [&hits, socket] {
                    const auto data = socket->property("headers").toByteArray() + socket->readAll();
                    socket->setProperty("headers", data);
                    if (socket->property("handled").toBool() || !data.contains("\r\n\r\n")) return;
                    socket->setProperty("handled", true);
                    const auto parts = data.left(data.indexOf("\r\n")).split(' ');
                    if (parts.size() < 2) { socket->abort(); return; }
                    const bool preflight = parts[0] == "OPTIONS";
                    if (!preflight) ++hits[parts[1]];
                    else ++hits["OPTIONS " + parts[1]];
                    const QByteArray body("<!doctype html><title>Probe ready</title>");
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Private-Network: true\r\nCache-Control: no-store\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
        return true;
    };
    QTcpServer source, target;
    if (!serve(source, kEngineProbeSourcePort) || !serve(target, kEngineProbeTargetPort)) {
        qCritical() << "ENGINE_PNA_PROBE FAIL: ports busy"; return 2;
    }
    QWebEngineProfile profile; // deliberately no RequestObserver: engine decision only
    QWebEnginePage page(&profile);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&page, &QWebEnginePage::titleChanged, &loop, [&](const QString &t) { if (t == "Probe complete") loop.quit(); });
    QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
        if (!ok) { qCritical() << "ENGINE_PNA_PROBE FAIL: source load"; loop.quit(); return; }
        page.runJavaScript(QString(R"JS((async()=>{
            const out={};
            for(const [k,u] of Object.entries({literal:'http://127.0.0.1:%1/literal',alias:'http://cybou-alias.test:%1/alias'})){
                try{await fetch(u,{mode:'no-cors',cache:'no-store'});out[k]='ok'}catch(e){out[k]='blocked:'+e.message}
            }
            window.__r=JSON.stringify(out);document.title='Probe complete';
        })())JS").arg(kEngineProbeTargetPort));
    });
    timeout.start(20000);
    page.load(QUrl(QString("http://cybou-source.test:%1/source").arg(kEngineProbeSourcePort)));
    loop.exec();
    QString result;
    QEventLoop get; page.runJavaScript("window.__r", [&](const QVariant &v) { result = v.toString(); get.quit(); });
    QTimer::singleShot(3000, &get, &QEventLoop::quit); get.exec();
    qInfo() << "ENGINE_PNA_RESULT" << result << "serverHits" << hits;
    return result.isEmpty() ? 2 : 0;
}
