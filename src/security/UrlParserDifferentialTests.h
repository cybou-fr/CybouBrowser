#pragma once
#include <QEventLoop>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "NavigationGuard.h"

// ASVS 1.5.3 browser analogue: the application classifies hosts with QUrl while the engine connects
// using its own URL parser. Every spelling that actually reaches the loopback fixture must also be
// classified private by NavigationGuard; otherwise a classification built on QUrl can be bypassed.
// --url-parser-probe reports the table; --url-parser-test asserts (any gap fails the test).
inline int runUrlParserDifferential(bool assertMode) {
    QMap<QByteArray, int> hits;
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&hits, socket] {
                const auto request = socket->property("request").toByteArray() + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);
                const auto parts = request.left(request.indexOf("\r\n")).split(' ');
                if (parts.size() > 1) ++hits[parts[1]];
                const QByteArray body("<!doctype html><title>fixture</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    const QList<QPair<QString, QString>> hosts = {
        {"canonical", "127.0.0.1"}, {"short", "127.1"}, {"hex-octet", "0x7f.0.0.1"}, {"decimal", "2130706433"},
        {"octal-octet", "0177.0.0.1"}, {"octal-int", "017700000001"}, {"hex-int", "0x7f000001"}, {"zero-padded", "127.000.000.001"},
        {"v4-mapped-v6", "[::ffff:127.0.0.1]"}, {"localhost", "localhost"}, {"localhost-dot", "localhost."},
        {"trailing-dot", "127.0.0.1."}, {"double-trailing-dot", "127.0.0.1.."}, {"ideographic-dots", QString::fromUtf8("127\xE3\x80\x82" "0\xE3\x80\x82" "0\xE3\x80\x82" "1")},
        {"fullwidth-digits", QString::fromUtf8("\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x97.0.0.1")}, {"any-address", "0.0.0.0"}, {"zero", "0"},
    };
    QWebEngineProfile profile; // engine only, no interceptor and no hardening: we want raw connect behaviour
    int gaps = 0;
    for (int i = 0; i < hosts.size(); ++i) {
        const QString path = QString("/p%1").arg(i);
        const QString spelled = QString("http://%1:%2%3").arg(hosts[i].second).arg(server.serverPort()).arg(path);
        QWebEnginePage page(&profile);
        QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, &QEventLoop::quit);
        timeout.start(8000);
        page.load(QUrl(spelled));
        loop.exec();
        const int reached = hits.value(path.toUtf8());
        const bool guardPrivate = NavigationGuard::isPrivateNetwork(QUrl(spelled));
        qInfo().noquote() << "URL_PARSER_CASE" << hosts[i].first << "engineReachedLoopback" << (reached > 0) << "guardSaysPrivate" << guardPrivate << "qurlHost" << QUrl(spelled).host();
        if (reached > 0 && !guardPrivate) {
            ++gaps;
            qWarning().noquote() << "URL_PARSER_KNOWN_GAP:" << hosts[i].first << "reaches loopback but NavigationGuard does not classify it private";
        }
    }
    if (assertMode) {
        // The control spellings must be classified correctly; otherwise the table is meaningless.
        for (const char *control : {"127.0.0.1", "localhost"}) {
            if (!NavigationGuard::isPrivateNetwork(QUrl(QString("http://%1:80/").arg(QString::fromLatin1(control))))) { qCritical() << "URL_PARSER_TEST FAIL: control not private" << control; return 1; }
        }
        if (gaps != 0) { qCritical() << "URL_PARSER_TEST FAIL: spellings that reach loopback but are not classified private:" << gaps; return 1; }
        if (!hits.contains("/p0")) { qCritical() << "URL_PARSER_TEST FAIL: canonical loopback control did not reach the fixture"; return 1; }
        qInfo() << "URL_PARSER_TEST PASS: controls reached loopback and are classified private; every spelling that reaches loopback is classified private";
    }
    return 0;
}
