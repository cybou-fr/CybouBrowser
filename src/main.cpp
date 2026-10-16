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
    app.setApplicationVersion("0.3.0");
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
            QObject::connect(window, &QQuickWindow::closing, window, [window] { window->deleteLater(); });
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
                if (state.toInt() < 0 || ++*attempts > 30 || success) {
                    poll->stop();
                    qInfo() << "CYBOU_SMOKE_TEST" << (success ? "PASS" : "FAIL");
                    QTimer::singleShot(700, &app, [root, &app, success] {
                        if (auto window = qobject_cast<QQuickWindow *>(root)) window->grabWindow().save("build/smoke-preview.png");
                        app.exit(success ? 0 : 2);
                    });
                }
            });
            poll->start(300);
        });
    }
    return app.exec();
}
