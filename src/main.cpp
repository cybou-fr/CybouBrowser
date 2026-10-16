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
#include <QTranslator>
#include <QtTest/QTest>
#include <memory>
#include "browserstore.h"
#include "security/SecurityTests.h"
#include "controlbar/ControlBarController.h"
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    app.setApplicationName("CYBOU");
    app.setOrganizationName("CYBOU");
    const bool controlBarTest = app.arguments().contains("--controlbar-test");
    const bool securityTest = app.arguments().contains("--security-test");
    const bool smokeTest = app.arguments().contains("--smoke-test") || controlBarTest || securityTest;
    if (smokeTest) app.setOrganizationName("CYBOU-Tests");
    app.setApplicationVersion("0.8.0");
    app.setWindowIcon(QIcon(":/assets/cybou.png"));
    QQuickStyle::setStyle("Basic");
    if (smokeTest) qInfo() << "ENGINE" << qVersion() << qWebEngineVersion() << qWebEngineChromiumVersion() << "security patch" << qWebEngineChromiumSecurityPatchVersion();
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
                    const bool controls = request.startsWith("GET /controls ");
                    const QByteArray body = download ? QByteArray("CYBOU download test\n") : controls ? QByteArray("<!doctype html><title>Control fixture</title><body style='background:#dfeaf4;color:#183757;font:20px sans-serif;height:1800px;padding:50px'><h1>CYBOU Control Bar</h1><p>Local engine fixture: independent policies, native permission decisions and bounded network metadata.</p><script>document.title='Scripts enabled';try{localStorage.setItem('cybouControl','1')}catch(e){document.title='Storage blocked'}</script>") : QByteArray("<!doctype html><title>CYBOU smoke page</title><h1>CYBOU test needle</h1>");
                    const auto cookie = request.startsWith("GET /page ") ? QByteArray("Set-Cookie: cybou_smoke=present; Path=/; SameSite=Lax\r\n") : QByteArray();
                    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: " + QByteArray(download ? "application/octet-stream" : "text/html") + "\r\n" + cookie + (download ? QByteArray("Content-Disposition: attachment; filename=cybou-smoke.txt\r\n") : QByteArray()) + "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
        QDir().mkpath("build/test-downloads");
    }
    if (securityTest) return runSecurityTests(app, testServer.serverPort());
    BrowserStore store;
    if (smokeTest) store.setHttpsOnly(false); // Explicit isolated test-profile HTTP fixture.
    QTranslator controlTranslator;
    QQmlApplicationEngine engine;
    auto translateControlBar = [&] {
        app.removeTranslator(&controlTranslator);
        if (store.controlBarLanguage() == "ru" && controlTranslator.load(":/translations/cybou_ru.qm")) app.installTranslator(&controlTranslator);
        engine.retranslate();
    };
    QObject::connect(&store, &BrowserStore::languageChanged, &engine, translateControlBar);
    translateControlBar();
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
    QString initialUrl;
    QString screenshotPath;
    for (int i = 1; i < app.arguments().size(); ++i) {
        const QString arg = app.arguments().at(i);
        if (arg == "--screenshot" && i + 1 < app.arguments().size()) {
            screenshotPath = app.arguments().at(++i);
        } else if (arg.startsWith("http://") || arg.startsWith("https://") || arg.startsWith("file://")) {
            initialUrl = arg;
        }
    }
    QVariantMap initialProps;
    if (app.arguments().contains("--private")) initialProps["privateMode"] = true;
    if (app.arguments().contains("--open-library")) initialProps["initialLibrarySection"] = "Téléchargements";
    if (app.arguments().contains("--open-settings")) initialProps["initialSettingsOpen"] = true;
    if (app.arguments().contains("--open-menu")) initialProps["initialMenuOpen"] = true;
    if (!initialUrl.isEmpty()) initialProps["initialUrlArgument"] = initialUrl;
    if (!initialProps.isEmpty()) engine.setInitialProperties(initialProps);
    engine.loadFromModule("Cybou", "Main");
    if (!screenshotPath.isEmpty() && !engine.rootObjects().isEmpty()) {
        auto root = engine.rootObjects().first();
        QTimer::singleShot(4000, &app, [root, &app, screenshotPath] {
            if (auto window = qobject_cast<QQuickWindow *>(root)) {
                window->grabWindow().save(screenshotPath);
                qInfo() << "SCREENSHOT_SAVED" << screenshotPath;
            }
            app.exit(0);
        });
    }
    if (smokeTest && !engine.rootObjects().isEmpty()) {
        auto root = engine.rootObjects().first();
        QTimer::singleShot(1500, &app, [root, &app, &engine, &store, controlBarTest] {
            auto dragWindow = qobject_cast<QQuickWindow *>(root);
            dragWindow->requestActivate(); QCoreApplication::processEvents();
            QTest::mouseMove(dragWindow, QPoint(80, 300));
            QVariant dragCoordinates;
            QMetaObject::invokeMethod(root, "prepareDragSmokeTest", Q_RETURN_ARG(QVariant, dragCoordinates));
            auto points = dragCoordinates.toMap();
            if (points.isEmpty()) points = dragCoordinates.value<QJSValue>().toVariant().toMap();
            const QPointF start(points["x1"].toDouble(), points["y1"].toDouble());
            const QPointF end(points["x2"].toDouble(), points["y2"].toDouble());
            dragWindow->requestActivate(); QCoreApplication::processEvents();
            QTest::mousePress(dragWindow, Qt::LeftButton, Qt::NoModifier, start.toPoint());
            QTest::mouseMove(dragWindow, (start + QPointF(12, 0)).toPoint());
            QTest::mouseMove(dragWindow, end.toPoint());
            QTest::mouseRelease(dragWindow, Qt::LeftButton, Qt::NoModifier, end.toPoint());
            QVariant dragged;
            QMetaObject::invokeMethod(root, "verifyDragSmokeTest", Q_RETURN_ARG(QVariant, dragged));
            if (!dragged.toBool()) { dragWindow->grabWindow().save("build/drag-failure.png"); qCritical() << "TAB_DRAG_TEST FAIL" << points << root->property("activeTab") << "popup" << dragWindow->activeFocusItem(); app.exit(2); return; }
            qInfo() << "TAB_DRAG_TEST PASS";
            QMetaObject::invokeMethod(root, "prepareSuggestionsSmokeTest");
            for (const char character : QByteArray("cybou-suggest")) QTest::keyClick(dragWindow, character);
            QTest::keyClick(dragWindow, Qt::Key_Down);
            QTest::keyClick(dragWindow, Qt::Key_Return);
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
            {
                ControlBarController policyA, policyB;
                for (auto controller : {&policyA, &policyB}) { controller->setStore(&store); controller->setUrl(QUrl("https://private-policy.example")); controller->setTabId("private-test"); }
                policyA.setProfile(profileOf(privateWindows[0])); policyB.setProfile(profileOf(privateWindows[1]));
                policyA.setBehavior("javascript", 2);
                policyA.notePopup("private-test", QUrl("https://private-policy.example"), true);
                if (policyA.settingFor(QUrl("https://private-policy.example"), "javascript") || !policyB.settingFor(QUrl("https://private-policy.example"), "javascript") || policyB.capability("popups").value("recent").toBool()) {
                    qCritical() << "PRIVATE_CONTROL_POLICY_TEST FAIL"; app.exit(2); return;
                }
                qInfo() << "PRIVATE_CONTROL_POLICY_TEST PASS";
            }
            const QPointer<QObject> privateProfileA(profileOf(privateWindows[0]));
            const QPointer<QObject> privateProfileB(profileOf(privateWindows[1]));
            for (auto window : privateWindows) window->close();
            qInfo() << "PRIVATE_WINDOW_TEST PASS";
            auto poll = new QTimer(&app);
            auto attempts = std::make_shared<int>(0);
            auto controlStarted = std::make_shared<bool>(false);
            QObject::connect(poll, &QTimer::timeout, &app, [root, &app, poll, attempts, privateProfileA, privateProfileB, controlBarTest, controlStarted] {
                QVariant state;
                if (*controlStarted) {
                    QMetaObject::invokeMethod(root, "pollControlBarTest", Q_RETURN_ARG(QVariant, state));
                    QVariant screenshot;
                    QMetaObject::invokeMethod(root, "takeControlBarScreenshot", Q_RETURN_ARG(QVariant, screenshot));
                    if (!screenshot.toString().isEmpty()) {
                        auto window = qobject_cast<QQuickWindow *>(root);
                        window->grabWindow().save("build/" + screenshot.toString() + ".png");
                        qInfo() << "CONTROL_BAR_SNAPSHOT" << screenshot.toString() << "DPR" << window->devicePixelRatio();
                    }
                    QVariant input;
                    QMetaObject::invokeMethod(root, "takeControlBarInput", Q_RETURN_ARG(QVariant, input));
                    auto data = input.toMap();
                    if (data.isEmpty()) data = input.value<QJSValue>().toVariant().toMap();
                    auto window = qobject_cast<QQuickWindow *>(root);
                    if (data.value("kind") == "key") {
                        const auto modifiers = Qt::KeyboardModifiers(data.value("modifiers", 0).toInt());
                        window->requestActivate(); QCoreApplication::processEvents();
                        QTest::keyClick(window, Qt::Key(data.value("key").toInt()), modifiers);
                    } else if (!data.isEmpty()) {
                        const QPointF point(data["x"].toDouble(), data["y"].toDouble());
                        QTest::mouseMove(window, point.toPoint());
                        if (data.value("kind") == "click") QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point.toPoint());
                    }
                } else {
                    const int smokeStage = root->property("smokeStage").toInt();
                    if (smokeStage == 21 || smokeStage == 23)
                        if (auto window = qobject_cast<QQuickWindow *>(root))
                            window->grabWindow().save(smokeStage == 21 ? "build/download-review.png" : "build/download-blocked.png");
                    QMetaObject::invokeMethod(root, "pollSmokeTest", Q_RETURN_ARG(QVariant, state));
                    if (state.toInt() == 2 && controlBarTest) {
                        QMetaObject::invokeMethod(root, "beginControlBarTest"); *controlStarted = true; state = 0;
                    }
                }
                if (state.toInt() == 1) return;
                const bool success = state.toInt() == 2 && privateProfileA.isNull() && privateProfileB.isNull();
                if (state.toInt() < 0 || ++*attempts > (controlBarTest ? 700 : 200) || success) {
                    poll->stop();
                    qInfo() << "CYBOU_SMOKE_TEST" << (success ? "PASS" : "FAIL");
                    if (success) {
                        QMetaObject::invokeMethod(root, "prepareFeaturePreview");
                        QTimer::singleShot(500, &app, [root, &app, success] {
                            if (auto window = qobject_cast<QQuickWindow *>(root))
                                window->grabWindow().save("build/cybou-08-alpine-dawn.png");
                            QMetaObject::invokeMethod(root, "showSuggestionsPreview");
                            QTimer::singleShot(500, &app, [root, &app, success] {
                                QFile pdf("build/test-downloads/cybou-smoke.pdf");
                                const bool validPdf = pdf.open(QIODevice::ReadOnly) && pdf.size() > 1000 && pdf.read(5) == "%PDF-";
                                qInfo() << "PDF_EXPORT_TEST" << (validPdf ? "PASS" : "FAIL");
                                if (auto window = qobject_cast<QQuickWindow *>(root)) window->grabWindow().save("build/smoke-preview.png");
                                if (success) QMetaObject::invokeMethod(root, "cleanFeaturePreview");
                                app.exit(success && validPdf ? 0 : 2);
                            });
                        });
                    } else {
                        app.exit(2);
                    }
                }
            });
            poll->start(300);
        });
    }
    return app.exec();
}
