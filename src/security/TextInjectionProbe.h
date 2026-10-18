#pragma once
#include <QEventLoop>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

// Does an untrusted string shown in a QML Text/Label make the application's own UI fetch a URL?
// Counts requests that reach a loopback server for three text formats. Diagnostic: it reports the
// Qt behaviour; the product QML is covered by tools/security/test_qml_text_lint.py.
inline int runTextInjectionProbe() {
    int hits = 0;
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [&hits, socket] {
                socket->readAll(); ++hits;
                socket->write("HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
                socket->disconnectFromHost();
            });
        }
    });
    int status = 0;
    for (const char *format : {"Text.AutoText", "Text.StyledText", "Text.PlainText"}) {
        hits = 0;
        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QString payload = QString("<img src=\"http://127.0.0.1:%1/pixel.png\" width=\"20\" height=\"20\"> page title").arg(server.serverPort());
        component.setData(QString("import QtQuick\nItem { width: 300; height: 60\n Text { objectName: 't'; width: 300; textFormat: %1; text: '%2' } }")
                              .arg(QString::fromLatin1(format), QString(payload).replace('\'', "\\'")).toUtf8(), QUrl());
        std::unique_ptr<QObject> root(component.create());
        auto item = qobject_cast<QQuickItem *>(root.get());
        if (!item) { qCritical() << "TEXT_INJECTION_PROBE FAIL: component" << component.errorString(); return 2; }
        QQuickWindow window;
        item->setParentItem(window.contentItem());
        window.resize(300, 60);
        window.show();
        QEventLoop loop; QTimer::singleShot(2500, &loop, &QEventLoop::quit); loop.exec();
        qInfo() << "TEXT_INJECTION_RESULT" << format << "requestsToLoopback" << hits;
        window.hide();
    }
    return status;
}
