#pragma once
#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "../controlbar/RequestObserver.h"

// Real HTTP redirects through the production interceptor; no Chromium security flags.
inline bool runHttpRedirectTests() {
    QTcpServer source, peer;
    if (!source.listen(QHostAddress::LocalHost) || !peer.listen(QHostAddress::LocalHost)) return false;
    int starts = 0, landings = 0;
    QUrl redirect;
    auto serve = [&](QTcpServer &server) {
        QObject::connect(&server, &QTcpServer::newConnection, &server, [&, listener=&server] {
            while (auto socket = listener->nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                    auto request = socket->property("request").toByteArray() + socket->readAll();
                    socket->setProperty("request", request);
                    if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
                    socket->setProperty("handled", true);
                    if (request.startsWith("GET /start?")) {
                        ++starts;
                        socket->write("HTTP/1.1 302 Found\r\nConnection: close\r\nLocation: " + redirect.toEncoded() + "\r\nContent-Length: 0\r\n\r\n");
                    } else if (request.startsWith("GET /landing ")) {
                        ++landings;
                        const QByteArray body("<!doctype html><title>Redirect landing</title>");
                        socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    }
                    else { socket->write("HTTP/1.1 204 No Content\r\nConnection: close\r\nContent-Length: 0\r\n\r\n"); }
                    socket->disconnectFromHost();
                });
            }
        });
    };
    serve(source); serve(peer);
    const QString origin = QString("http://127.0.0.1:%1").arg(source.serverPort());
    const QString peerOrigin = QString("http://127.0.0.1:%1").arg(peer.serverPort());
    const QStringList scenarios{"direct", "peer-direct", "same-origin", "query-change", "loop", "cross-origin", "credentials"};
    for (const auto &scenario : scenarios) {
        starts = 0; landings = 0;
        const bool direct = scenario == "direct" || scenario == "peer-direct";
        const QUrl initial(direct ? (scenario == "direct" ? origin : peerOrigin) + "/landing"
                                 : origin + "/start?consent=one");
        if (scenario == "same-origin") redirect = QUrl(origin + "/landing");
        if (scenario == "query-change") redirect = QUrl(origin + "/start?consent=two");
        if (scenario == "loop") redirect = initial;
        if (scenario == "cross-origin") redirect = QUrl(peerOrigin + "/landing");
        if (scenario == "credentials") { redirect = QUrl(origin + "/landing"); redirect.setUserName("fixture-user"); redirect.setPassword("fixture-secret"); }
        QWebEngineProfile profile;
        RequestObserver observer(&profile);
        observer.setHttpsOnly(true);
        profile.setUrlRequestInterceptor(&observer);
        if (!observer.allowHttpNavigation(initial)) return false;
        QWebEnginePage page(&profile);
        QEventLoop loop;
        QTimer deadline;
        deadline.setSingleShot(true);
        bool finished = false, loaded = false;
        QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) { finished = true; loaded = ok; loop.quit(); });
        deadline.start(10000); page.load(initial); loop.exec(); deadline.stop();
        if (!finished || loaded != direct || starts != (direct ? 0 : 1) || landings != (direct ? 1 : 0)) {
            qCritical() << "HTTP_REDIRECT_TEST FAIL" << scenario << "finished" << finished << "loaded" << loaded << "starts" << starts << "landings" << landings;
            return false;
        }
        if (!direct) {
            bool denied = false;
            for (const auto &value : observer.sample()) {
                const auto event = value.toMap();
                const auto expected = scenario == "credentials" ? QString("embedded-credentials") : QString("insecure-transport");
                if (event.value("blocked").toBool() && event.value("reason").toString() == expected) denied = true;
                if (event.value("origin").toString().contains("fixture-") || event.value("origin").toString().contains("consent=")) {
                    qCritical() << "HTTP_REDIRECT_TEST FAIL: unsanitized observation"; return false;
                }
            }
            if (!denied) { qCritical() << "HTTP_REDIRECT_TEST FAIL: missing native deny observation" << scenario; return false; }
        }
        qInfo() << "HTTP_REDIRECT_CASE PASS" << scenario;
    }
    qInfo() << "HTTP_REDIRECT_TEST PASS: two live positive servers; 302 path/query/loop/cross-origin/credentials never inherit one-shot HTTP consent";
    return true;
}
