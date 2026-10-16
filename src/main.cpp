#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QIcon>
#include <QTimer>
#include <QQuickWindow>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QJSValue>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFile>
#include <memory>
#include "browserstore.h"
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    app.setApplicationName("CYBOU");
    app.setOrganizationName("CYBOU");
    const bool smokeTest = app.arguments().contains("--smoke-test");
    if (smokeTest) app.setOrganizationName("CYBOU-Tests");
    app.setApplicationVersion("0.5.0");
    app.setWindowIcon(QIcon(":/assets/cybou.png"));
    QQuickStyle::setStyle("Basic");
    QTcpServer testServer;
    if (smokeTest) {
        testServer.listen(QHostAddress::LocalHost);
        QObject::connect(&testServer, &QTcpServer::newConnection, &app, [&testServer] {
            while (auto socket = testServer.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                    const auto request = socket->readAll();
                    if (request.startsWith("GET /slow-download ")) {
                        const qint64 size = 4 * 1024 * 1024;
                        const auto match = QRegularExpression("Range: bytes=(\\d+)-", QRegularExpression::CaseInsensitiveOption).match(QString::fromLatin1(request));
                        const qint64 start = match.hasMatch() ? match.captured(1).toLongLong() : 0;
                        socket->write(QByteArray(match.hasMatch() ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n")
                            + "Connection: close\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: bytes\r\nContent-Disposition: attachment; filename=cybou-slow.bin\r\n"
                            + (match.hasMatch() ? "Content-Range: bytes " + QByteArray::number(start) + "-" + QByteArray::number(size - 1) + "/" + QByteArray::number(size) + "\r\n" : QByteArray())
                            + "Content-Length: " + QByteArray::number(size - start) + "\r\n\r\n");
                        auto sent = std::make_shared<qint64>(start);
                        auto transfer = new QTimer(socket);
                        QObject::connect(transfer, &QTimer::timeout, socket, [socket, transfer, sent, size] {
                            const qint64 count = qMin<qint64>(65536, size - *sent);
                            socket->write(QByteArray(count, 'C')); *sent += count;
                            if (*sent >= size) { transfer->stop(); socket->disconnectFromHost(); }
                        });
                        transfer->start(40); return;
                    }
                    const bool download = request.startsWith("GET /download ");
                    const QByteArray body = download ? QByteArray("CYBOU download test\n") : QByteArray("<!doctype html><title>CYBOU smoke page</title><h1>CYBOU test needle</h1>");
                    const auto cookie = request.startsWith("GET /page ") ? QByteArray("Set-Cookie: cybou_smoke=present; Path=/; SameSite=Lax\r\n") : QByteArray();
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: " + QByteArray(download ? "application/octet-stream" : "text/html") + "\r\n" + cookie + (download ? QByteArray("Content-Disposition: attachment; filename=cybou-smoke.txt\r\n") : QByteArray()) + "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
        QDir().mkpath("build/test-downloads");
    }
    BrowserStore store;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("browserStore", &store);
    engine.rootContext()->setContextProperty("smokeMode", smokeTest);
    engine.rootContext()->setContextProperty("smokeBaseUrl", QString("http://127.0.0.1:%1").arg(testServer.serverPort()));
    engine.rootContext()->setContextProperty("smokeDownloadPath", QDir::current().absoluteFilePath("build/test-downloads"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [](QObject *object, const QUrl &) {
        if (auto window = qobject_cast<QQuickWindow *>(object))
            QObject::connect(window, &QQuickWindow::closing, window, [window] {
                QTimer::singleShot(0, window, [window] { if (!window->isVisible()) window->deleteLater(); });
            });
    });
    QObject::connect(&store, &BrowserStore::privateWindowRequested, &engine, [&engine] {
        engine.setInitialProperties({{"privateMode", true}});
        engine.loadFromModule("Cybou", "Main");
        engine.setInitialProperties({});
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    if (app.arguments().contains("--private")) engine.setInitialProperties({{"privateMode", true}});
    engine.loadFromModule("Cybou", "Main");
    if (smokeTest && !engine.rootObjects().isEmpty()) {
        auto root = engine.rootObjects().first();
        QTimer::singleShot(1500, &app, [root, &app, &engine] {
            QVariant dragCoordinates;
            QMetaObject::invokeMethod(root, "prepareDragSmokeTest", Q_RETURN_ARG(QVariant, dragCoordinates));
            auto points = dragCoordinates.toMap();
            if (points.isEmpty()) points = dragCoordinates.value<QJSValue>().toVariant().toMap();
            auto dragWindow = qobject_cast<QQuickWindow *>(root);
            const QPointF start(points["x1"].toDouble(), points["y1"].toDouble());
            const QPointF end(points["x2"].toDouble(), points["y2"].toDouble());
            auto sendMouse = [dragWindow](QEvent::Type type, const QPointF &point, Qt::MouseButton button, Qt::MouseButtons buttons) {
                QMouseEvent event(type, point, dragWindow->mapToGlobal(point.toPoint()), button, buttons, Qt::NoModifier);
                QCoreApplication::sendEvent(dragWindow, &event);
                QCoreApplication::processEvents();
            };
            sendMouse(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
            sendMouse(QEvent::MouseMove, start + QPointF(12, 0), Qt::NoButton, Qt::LeftButton);
            sendMouse(QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
            sendMouse(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
            QVariant dragged;
            QMetaObject::invokeMethod(root, "verifyDragSmokeTest", Q_RETURN_ARG(QVariant, dragged));
            if (!dragged.toBool()) { qCritical() << "TAB_DRAG_TEST FAIL" << points; app.exit(2); return; }
            qInfo() << "TAB_DRAG_TEST PASS";
            QMetaObject::invokeMethod(root, "prepareSuggestionsSmokeTest");
            auto sendKey = [dragWindow](int key, const QString &text = QString()) {
                QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, text);
                QCoreApplication::sendEvent(dragWindow, &press);
                QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, text);
                QCoreApplication::sendEvent(dragWindow, &release);
                QCoreApplication::processEvents();
            };
            sendKey(Qt::Key_unknown, "cybou-suggest");
            sendKey(Qt::Key_Down);
            sendKey(Qt::Key_Return);
            QVariant suggestionResult;
            QMetaObject::invokeMethod(root, "verifySuggestionsSmokeTest", Q_RETURN_ARG(QVariant, suggestionResult));
            if (!suggestionResult.toBool()) { qCritical() << "ADDRESS_SUGGESTIONS_TEST FAIL"; app.exit(2); return; }
            qInfo() << "ADDRESS_SUGGESTIONS_TEST PASS";
            QVariant result;
            QMetaObject::invokeMethod(root, "runSmokeTest", Q_RETURN_ARG(QVariant, result));
            const bool passed = result.toBool();
            if (!passed) { app.exit(2); return; }
            QList<QQuickWindow *> privateWindows;
            for (auto object : engine.rootObjects()) {
                if (!object->property("privateMode").toBool()) continue;
                QVariant privateResult;
                QMetaObject::invokeMethod(object, "runPrivateSmokeTest", Q_RETURN_ARG(QVariant, privateResult));
                if (!privateResult.toBool()) { qCritical() << "PRIVATE_WINDOW_TEST FAIL"; app.exit(2); return; }
                privateWindows.append(qobject_cast<QQuickWindow *>(object));
            }
            auto profileOf = [](QObject *object) {
                const auto value = object->property("privateProfile");
                if (auto profile = value.value<QObject *>()) return profile;
                return value.value<QJSValue>().toQObject();
            };
            if (privateWindows.size() != 2 || !profileOf(privateWindows[0]) || profileOf(privateWindows[0]) == profileOf(privateWindows[1])) {
                qCritical() << "PRIVATE_PROFILE_ISOLATION FAIL"; app.exit(2); return;
            }
            const QPointer<QObject> privateProfileA(profileOf(privateWindows[0]));
            const QPointer<QObject> privateProfileB(profileOf(privateWindows[1]));
            for (auto window : privateWindows) window->close();
            qInfo() << "PRIVATE_WINDOW_TEST PASS";
            auto poll = new QTimer(&app);
            auto attempts = std::make_shared<int>(0);
            QObject::connect(poll, &QTimer::timeout, &app, [root, &app, poll, attempts, privateProfileA, privateProfileB] {
                QVariant state;
                QMetaObject::invokeMethod(root, "pollSmokeTest", Q_RETURN_ARG(QVariant, state));
                if (state.toInt() == 1) return;
                const bool success = state.toInt() == 2 && privateProfileA.isNull() && privateProfileB.isNull();
                if (state.toInt() < 0 || ++*attempts > 120 || success) {
                    poll->stop();
                    qInfo() << "CYBOU_SMOKE_TEST" << (success ? "PASS" : "FAIL");
                    if (success) QMetaObject::invokeMethod(root, "prepareFeaturePreview");
                    QTimer::singleShot(700, &app, [root, &app, success] {
                        QFile pdf("build/test-downloads/cybou-smoke.pdf");
                        const bool validPdf = pdf.open(QIODevice::ReadOnly) && pdf.size() > 1000 && pdf.read(5) == "%PDF-";
                        qInfo() << "PDF_EXPORT_TEST" << (validPdf ? "PASS" : "FAIL");
                        if (auto window = qobject_cast<QQuickWindow *>(root)) window->grabWindow().save("build/smoke-preview.png");
                        if (success) QMetaObject::invokeMethod(root, "cleanFeaturePreview");
                        app.exit(success && validPdf ? 0 : 2);
                    });
                }
            });
            poll->start(300);
        });
    }
    return app.exec();
}
