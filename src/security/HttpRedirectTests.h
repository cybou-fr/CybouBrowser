#pragma once
#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineHttpRequest>
#include "TlsBoundaryTests.h"
#include <QtWebEngineCore/QWebEngineProfile>
#include "../controlbar/RequestObserver.h"

// Real HTTP redirects through the production interceptor; no Chromium security flags.
inline bool runHttpRedirectTests() {
    QTcpServer source, peer;
    if (!source.listen(QHostAddress::LocalHost) || !peer.listen(QHostAddress::LocalHost)) return false;
    int starts = 0, landings = 0;
    int status = 302;
    QByteArray observedMethod, observedBody;
    QUrl redirect;
    auto serve = [&](QTcpServer &server) {
        QObject::connect(&server, &QTcpServer::newConnection, &server, [&, listener=&server] {
            while (auto socket = listener->nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                    auto request = socket->property("request").toByteArray() + socket->readAll();
                    socket->setProperty("request", request);
                    if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
                    const int headerEnd = request.indexOf("\r\n\r\n") + 4;
                    int length = 0;
                    for (const auto &line : request.left(headerEnd).split('\n'))
                        if (line.toLower().startsWith("content-length:")) length = line.mid(line.indexOf(':') + 1).trimmed().toInt();
                    if (request.size() < headerEnd + length) return;
                    socket->setProperty("handled", true);
                    const auto parts = request.left(request.indexOf("\r\n")).split(' ');
                    if (parts.size() < 2) { socket->abort(); return; }
                    if (parts[1].startsWith("/start?")) {
                        ++starts;
                        observedMethod = parts[0]; observedBody = request.mid(headerEnd, length);
                        socket->write("HTTP/1.1 " + QByteArray::number(status) + " Redirect\r\nConnection: close\r\nLocation: " + redirect.toEncoded() + "\r\nContent-Length: 0\r\n\r\n");
                    } else if (parts[1] == "/landing") {
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
    int checked = 0;
    for (int code : {301, 302, 303, 307, 308}) for (bool post : {false, true}) for (const auto &scenario : scenarios) {
        status = code;
        starts = 0; landings = 0; observedMethod.clear(); observedBody.clear();
        const bool direct = scenario == "direct" || scenario == "peer-direct";
        if (direct && code != 302) continue;
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
        bool finished = false, loaded = false, mainFrameDenied = false;
        QTimer denialSettle;
        denialSettle.setSingleShot(true);
        QObject::connect(&denialSettle, &QTimer::timeout, &loop, &QEventLoop::quit);
        if (!direct) QObject::connect(&observer, &RequestObserver::activityAvailable, &loop, [&] {
            const QString expected = scenario == "credentials" ? "embedded-credentials" : "insecure-transport";
            for (const auto &value : observer.sample()) {
                const auto event = value.toMap();
                if (!mainFrameDenied && event.value("resourceType").toInt() == int(QWebEngineUrlRequestInfo::ResourceTypeMainFrame)
                    && event.value("blocked").toBool() && event.value("reason").toString() == expected) {
                    mainFrameDenied = true;
                    // The network decision is synchronous; renderer error-page completion can lag.
                    denialSettle.start(200);
                }
            }
        });
        QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) { finished = true; loaded = ok; loop.quit(); });
        QWebEngineHttpRequest navigation(initial);
        if (post) { navigation.setMethod(QWebEngineHttpRequest::Post); navigation.setPostData("fixture-body"); navigation.setHeader("Content-Type", "text/plain"); }
        deadline.start(10000); page.load(navigation); loop.exec(); deadline.stop();
        const bool completed = direct ? finished : (finished || mainFrameDenied);
        if (!completed || loaded != direct || starts != (direct ? 0 : 1) || landings != (direct ? 1 : 0)) {
            qCritical() << "HTTP_REDIRECT_TEST FAIL" << scenario << "finished" << finished << "loaded" << loaded << "starts" << starts << "landings" << landings << "nativeDenial" << mainFrameDenied << observer.sample();
            return false;
        }
        if (!direct) {
            if (observedMethod != (post ? "POST" : "GET") || observedBody != (post ? "fixture-body" : "")) {
                qCritical() << "HTTP_REDIRECT_TEST FAIL: initial method/body control" << scenario << code << post; return false;
            }
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
        denialSettle.stop();
        if (!finished) page.triggerAction(QWebEnginePage::Stop);
        ++checked;
        qInfo() << "HTTP_REDIRECT_CASE PASS" << scenario << "status" << code << "method" << (post ? "POST" : "GET");
    }
    if (checked != 54) return false;
    qInfo() << "HTTP_REDIRECT_TEST PASS: 54 GET/POST cases; 301/302/303/307/308 path/query/loop/cross-origin/credentials never inherit one-shot HTTP consent";
    return true;
}


inline bool runHttpsDowngradeRedirectTests() {
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
    qCritical() << "HTTPS_DOWNGRADE_REDIRECT_TEST BLOCKED: profile-local CA requires Qt 6.10+"; return false;
#else
    QFile rootFile("tests/security/fixtures/tls/test-ca-cert.pem"), leafFile("tests/security/fixtures/tls/valid-cert.pem"), keyFile("tests/security/fixtures/tls/localhost-key.pem");
    if (!rootFile.open(QIODevice::ReadOnly) || !leafFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) return false;
    const QSslCertificate root(rootFile.readAll()), leaf(leafFile.readAll());
    const QSslKey key(keyFile.readAll(), QSsl::Rsa);
    if (root.isNull() || leaf.isNull() || key.isNull()) return false;
    QTcpServer insecure;
    if (!insecure.listen(QHostAddress::LocalHost)) return false;
    int landed = 0;
    QObject::connect(&insecure, &QTcpServer::newConnection, &insecure, [&] {
        while (auto socket = insecure.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                auto request = socket->property("request").toByteArray() + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);
                const auto parts = request.left(request.indexOf("\r\n")).split(' ');
                if (parts.size() >= 2 && parts[1] == "/landing") ++landed;
                const QByteArray body("<!doctype html><title>Insecure landing</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    // Prove the HTTP endpoint is live; this deliberately permissive profile is test-only.
    {
        QWebEngineProfile profile; QWebEnginePage page(&profile); QEventLoop loop; QTimer deadline;
        deadline.setSingleShot(true); bool ok = false;
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool value) { ok = value; loop.quit(); });
        QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        deadline.start(10000); page.load(QUrl(QString("http://127.0.0.1:%1/landing").arg(insecure.serverPort()))); loop.exec();
        if (!ok || landed < 1) { qCritical() << "HTTPS_DOWNGRADE_REDIRECT_TEST FAIL: HTTP positive control"; return false; }
    }
    for (int code : {301,302,303,307,308}) for (bool post : {false,true}) {
        BoundaryTlsServer secure;
        secure.certificate = leaf; secure.key = key; secure.redirectStatus = code;
        secure.redirectLocation = QString("http://127.0.0.1:%1/landing").arg(insecure.serverPort()).toUtf8();
        if (!secure.listen(QHostAddress::LocalHost)) return false;
        QWebEngineProfileBuilder builder; builder.setAdditionalTrustedCertificates({root});
        std::unique_ptr<QWebEngineProfile> profile(builder.createProfile(QString()));
        if (!profile || !profile->isOffTheRecord()) return false;
        RequestObserver observer(profile.get()); observer.setHttpsOnly(true); profile->setUrlRequestInterceptor(&observer);
        QWebEnginePage page(profile.get()); QEventLoop loop; QTimer deadline; deadline.setSingleShot(true);
        bool loaded = false, finished = false; int certificateErrors = 0;
        QObject::connect(&page, &QWebEnginePage::certificateError, &loop, [&](QWebEngineCertificateError error) { ++certificateErrors; error.rejectCertificate(); });
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) { loaded = ok; finished = true; loop.quit(); });
        QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        const QString origin = QString("https://127.0.0.1:%1").arg(secure.serverPort());
        deadline.start(10000); page.load(QUrl(origin + "/page")); loop.exec(); deadline.stop();
        if (!finished || !loaded || certificateErrors || secure.httpRequests != 1) { qCritical() << "HTTPS_DOWNGRADE_REDIRECT_TEST FAIL: HTTPS positive control"; return false; }
        // Let any positive-control favicon finish before counting insecure endpoint traffic.
        const int before = landed;
        finished = false; loaded = false;
        QWebEngineHttpRequest navigation(QUrl(origin + "/redirect"));
        if (post) { navigation.setMethod(QWebEngineHttpRequest::Post); navigation.setPostData("downgrade-body"); navigation.setHeader("Content-Type","text/plain"); }
        deadline.start(10000); page.load(navigation); loop.exec(); deadline.stop();
        bool blocked = false;
        for (const auto &value : observer.sample()) {
            const auto event = value.toMap();
            if (event.value("blocked").toBool() && event.value("reason").toString() == "insecure-transport") blocked = true;
        }
        if (!finished || loaded || certificateErrors || secure.redirectRequests != 1 || landed != before || !blocked) {
            qCritical() << "HTTPS_DOWNGRADE_REDIRECT_TEST FAIL" << code << post << finished << loaded << certificateErrors << secure.redirectRequests << landed-before << blocked;
            return false;
        }
        qInfo() << "HTTPS_DOWNGRADE_REDIRECT_CASE PASS" << code << (post ? "POST" : "GET");
    }
    qInfo() << "HTTPS_DOWNGRADE_REDIRECT_TEST PASS: live HTTP/HTTPS controls; ten GET/POST redirects blocked before insecure endpoint";
    return true;
#endif
}
