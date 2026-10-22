#pragma once
#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QDebug>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEnginePermission>
#include <QtWebEngineCore/QWebEngineProfile>

// Diagnostic only: what does the raw engine (no CYBOU coordinator) give a page that asks for the camera, with the
// request granted at once, granted after a delay, or denied? Prints MEDIA_PROBE lines; exit 0 means it ran.
inline int runMediaProbe() {
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                socket->readAll();
                const QByteArray body("<!doctype html><title>probe</title>");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    QWebEngineProfile profile;
    profile.setPersistentPermissionsPolicy(QWebEngineProfile::PersistentPermissionsPolicy::AskEveryTime);
    for (const QString mode : {"grant", "grant-later", "deny", "grant-after-deny"}) {
        QWebEnginePage page(&profile);
        QString result = "timeout";
        QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&page, &QWebEnginePage::permissionRequested, &page, [&](QWebEnginePermission permission) {
            qInfo() << "MEDIA_PROBE request" << mode << int(permission.permissionType()) << permission.origin();
            if (mode == "grant" || mode == "grant-after-deny") permission.grant();
            else if (mode == "deny") permission.deny();
            else QTimer::singleShot(1500, &page, [permission]() mutable { permission.grant(); });
        });
        QObject::connect(&page, &QWebEnginePage::titleChanged, &loop, [&](const QString &t) {
            if (t.startsWith("r:")) { result = t.mid(2); loop.quit(); }
        });
        QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
            if (!ok) { result = "load-failed"; loop.quit(); return; }
            page.runJavaScript(QStringLiteral("navigator.mediaDevices.getUserMedia({video:true})"
                ".then(s=>document.title='r:live-'+s.getVideoTracks()[0].readyState)"
                ".catch(e=>document.title='r:'+e.name+'-'+e.message)"));
        });
        timeout.start(20000);
        page.load(QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort())));
        loop.exec();
        qInfo().noquote() << "MEDIA_PROBE_RESULT" << mode << result;
    }
    return 0;
}
