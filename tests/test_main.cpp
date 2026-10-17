#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QQmlComponent>
#include <QIcon>
#include <QTimer>
#include <QQuickWindow>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSet>
#include <QCryptographicHash>
#include <QDir>
#include <QJSValue>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFile>
#include <QTranslator>
#include <QtTest/QTest>
#include <memory>
#include <cstdio>
#include "browserstore.h"
#include "security/SecurityTests.h"
#include "controlbar/ControlBarController.h"
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &msg) {
        const auto diagnostic = (msg + QLatin1Char('\n')).toUtf8();
        std::fwrite(diagnostic.constData(), 1, size_t(diagnostic.size()), stderr);
        std::fflush(stderr);
        static QFile logFile("build/test_log.txt");
        if (!logFile.isOpen()) {
            QDir().mkpath(QStringLiteral("build"));
            std::ignore = logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        }
        QTextStream out(&logFile);
        out << msg << "\n";
        out.flush();
    });
    bool trackerTest = false;
    for (int i = 1; i < argc; ++i) trackerTest |= QByteArray(argv[i]) == "--tracker-test";
    if (trackerTest) qputenv("QTWEBENGINE_CHROMIUM_FLAGS", qgetenv("QTWEBENGINE_CHROMIUM_FLAGS") + " --host-resolver-rules=\"MAP google-analytics.com 127.0.0.1\" --no-proxy-server");
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    app.setApplicationName("CYBOU-Tests");
    app.setOrganizationName("CYBOU-Tests");
    app.setApplicationVersion(QStringLiteral(CYBOU_PRODUCT_VERSION));
    app.setWindowIcon(QIcon(":/assets/cybou.png"));
    QQuickStyle::setStyle("Basic");

    const bool controlBarTest = app.arguments().contains("--controlbar-test");
    const bool transportTest = app.arguments().contains("--transport-test") || trackerTest;
    const bool surfaceTest = app.arguments().contains("--surface-test");
    const bool securityTest = app.arguments().contains("--security-test");
    const bool explicitSmoke = app.arguments().contains("--smoke-test");
    
    // Default to running security and smoke tests if no arguments are provided
    const bool runAll = (argc == 1);
    const bool smokeTest = explicitSmoke || controlBarTest || transportTest || surfaceTest || runAll;

    qInfo() << "ENGINE" << qVersion() << qWebEngineVersion() << qWebEngineChromiumVersion() << "security patch" << qWebEngineChromiumSecurityPatchVersion();

    BoundaryTlsServer transportTls;
    QString transportWssUrl;
    if (transportTest && !trackerTest) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        QFile leaf("tests/security/fixtures/tls/valid-cert.pem"), key("tests/security/fixtures/tls/localhost-key.pem");
        if (!leaf.open(QIODevice::ReadOnly) || !key.open(QIODevice::ReadOnly)) return 2;
        transportTls.certificate = QSslCertificate(leaf.readAll()); transportTls.key = QSslKey(key.readAll(), QSsl::Rsa);
        if (!QSslSocket::supportsSsl() || transportTls.certificate.isNull() || transportTls.key.isNull() || !transportTls.listen(QHostAddress::LocalHost)) return 2;
        transportWssUrl = QString("wss://127.0.0.1:%1/socket").arg(transportTls.serverPort());
#else
        qCritical() << "TRANSPORT_WSS_TEST BLOCKED: Qt 6.10+ required"; return 2;
#endif
    }
    QSet<QTcpSocket *> activeTransportStreams;
    int transportStreamsOpened = 0;
    int transportStreamsClosed = 0;
    int transportWsRequests = 0;
    QTcpServer testServer;
    testServer.listen(QHostAddress::LocalHost);
    QObject::connect(&testServer, &QTcpServer::newConnection, &app, [&testServer, &transportWssUrl, &activeTransportStreams, &transportStreamsOpened, &transportStreamsClosed, &transportWsRequests] {
        while (auto socket = testServer.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, &transportWssUrl, &activeTransportStreams, &transportStreamsOpened, &transportStreamsClosed, &transportWsRequests] {
                const auto request = socket->readAll();
                if (request.startsWith("GET /transport-stream ")) {
                    activeTransportStreams.insert(socket);
                    ++transportStreamsOpened;
                    QObject::connect(socket, &QTcpSocket::disconnected, socket, [socket, &activeTransportStreams, &transportStreamsClosed] {
                        if (activeTransportStreams.remove(socket)) ++transportStreamsClosed;
                    });
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nTransfer-Encoding: chunked\r\nCache-Control: no-store\r\n\r\n1\r\nx\r\n");
                    // Deliberately never finish the body: only client teardown should disconnect it.
                    return;
                }
                if (request.startsWith("GET /transport-socket ")) {
                    ++transportWsRequests;
                    QByteArray key;
                    for (const auto &line : request.split('\n'))
                        if (line.toLower().startsWith("sec-websocket-key:")) key = line.mid(line.indexOf(':') + 1).trimmed();
                    const auto digest = QCryptographicHash::hash(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", QCryptographicHash::Sha1).toBase64();
                    socket->write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + digest + "\r\n\r\n");
                    return;
                }
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
                const bool transport = request.startsWith("GET /transport-document?");
                const bool controls = request.startsWith("GET /controls ");
                const bool tracker = request.startsWith("GET /tracker-document?");
                const QByteArray trackerBody = QByteArray("<!doctype html><title>Tracker fixture</title><h1>Tracker filter fixture</h1><script>fetch('http://google-analytics.com:") + QByteArray::number(socket->localPort()) + QByteArray("/tracker-asset?private=query').then(r=>r.text()).then(()=>document.title='Tracker allowed').catch(()=>document.title='Tracker blocked')</script>");
                const QByteArray wssProbe = transportWssUrl.isEmpty() ? QByteArray("window.transportWssReady=Promise.resolve();")
                    : QByteArray("window.transportWssReady=new Promise((resolve,reject)=>{window.transportSecureSocket=new WebSocket('") + transportWssUrl.toUtf8()
                    + QByteArray("');transportSecureSocket.onmessage=e=>{if(e.data==='live')resolve()};transportSecureSocket.onerror=()=>reject(Error('wss fixture failed'))});");
                const QByteArray body = tracker ? trackerBody : download ? QByteArray("CYBOU download test\n") : transport ? QByteArray("<!doctype html><title>Transport document</title><h1>Temporary HTTP fixture</h1><script>") + wssProbe + QByteArray("Promise.all([window.transportWssReady,fetch('/transport-asset'),fetch('/transport-stream').then(r=>{window.transportReader=r.body.getReader();return window.transportReader.read()}),new Promise((resolve,reject)=>{const s=new WebSocket(location.origin.replace('http:','ws:')+'/transport-socket');s.onerror=()=>resolve();s.onopen=()=>{s.close();reject(Error('ws escaped'))}}),fetch('http://localhost:") + QByteArray::number(socket->localPort()) + QByteArray("/cross-origin-http').catch(()=>null)]).then(()=>document.title='Transport ready')</script>") : controls ? QByteArray("<!doctype html><title>Control fixture</title><body style='background:#dfeaf4;color:#183757;font:20px sans-serif;height:1800px;padding:50px'><h1>CYBOU Control Bar</h1><p>Local engine fixture: independent policies, native permission decisions and bounded network metadata.</p><script>document.title='Scripts enabled';try{localStorage.setItem('cybouControl','1')}catch(e){document.title='Storage blocked'}</script>") : QByteArray("<!doctype html><title>CYBOU smoke page</title><h1>CYBOU test needle</h1>");
                const auto cookie = request.startsWith("GET /page ") ? QByteArray("Set-Cookie: cybou_smoke=present; Path=/; SameSite=Lax\r\n") : QByteArray();
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: " + QByteArray(download ? "application/octet-stream" : "text/html") + "\r\n" + cookie + (download ? QByteArray("Content-Disposition: attachment; filename=cybou-smoke.txt\r\n") : QByteArray()) + "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    QDir().mkpath("build/test-downloads");

    if (securityTest || runAll) {
        const int secResult = runSecurityTests(app, testServer.serverPort());
        if (secResult != 0 || securityTest) return secResult;
    }

    QQmlEngine profileFactoryEngine;
    BrowserStore::TemporaryProfileFactory transportFactory;
    if (!transportWssUrl.isEmpty()) {
        transportFactory = [&profileFactoryEngine](QObject *parent) -> BrowserStore::TemporaryProfileContext {
            QQmlComponent component(&profileFactoryEngine);
            component.setData("import QtWebEngine\nWebEngineProfilePrototype { persistentPermissionsPolicy: WebEngineProfile.AskEveryTime }", QUrl());
            auto prototype = component.createWithInitialProperties({{"additionalTrustedCertificateFiles", QStringList{QDir::current().absoluteFilePath("tests/security/fixtures/tls/test-ca-cert.pem")}}});
            if (!prototype) { qCritical() << "TRANSPORT_WSS_TEST factory failed" << component.errors(); return {}; }
            prototype->setParent(parent);
            QQuickWebEngineProfile *profile = nullptr;
            if (!QMetaObject::invokeMethod(prototype, "instance", Q_RETURN_ARG(QQuickWebEngineProfile *, profile)) || !profile) {
                delete prototype; return {};
            }
            QQmlEngine::setObjectOwnership(prototype, QQmlEngine::CppOwnership);
            return {profile, prototype};
        };
    }
    BrowserStore store(nullptr, std::move(transportFactory));
    store.allowTestHttpFixture();
    store.setHttpsOnly(false); // Explicit isolated test-profile HTTP fixture.
    store.setHttpsFirst(true);

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
    engine.rootContext()->setContextProperty("trackerTestMode", trackerTest);
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

    engine.loadFromModule("Cybou", "Main");

    if (smokeTest && !engine.rootObjects().isEmpty()) {
        auto root = engine.rootObjects().first();
        QTimer::singleShot(1500, &app, [root, &app, &engine, &store, &activeTransportStreams, &transportStreamsOpened, &transportStreamsClosed, &transportWsRequests, &transportTls, controlBarTest, transportTest, trackerTest, surfaceTest] {
            auto dragWindow = qobject_cast<QQuickWindow *>(root);
            dragWindow->resize(1360, 900);
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
            dragWindow->showNormal();
            dragWindow->resize(1100, 720);
            QTest::qWait(100);
            const QSize restoredSize = dragWindow->size();
            QVariant titleCoordinates;
            QMetaObject::invokeMethod(root, "testTitleDragPoint", Q_RETURN_ARG(QVariant, titleCoordinates));
            auto titlePoint = titleCoordinates.toMap();
            if (titlePoint.isEmpty()) titlePoint = titleCoordinates.value<QJSValue>().toVariant().toMap();
            const QPoint titlePosition(titlePoint["x"].toInt(), titlePoint["y"].toInt());
            const QPoint restoredPosition = dragWindow->position();
            QTest::mousePress(dragWindow, Qt::LeftButton, Qt::NoModifier, titlePosition);
            QTest::mouseMove(dragWindow, titlePosition + QPoint(1, 0));
            QTest::mouseRelease(dragWindow, Qt::LeftButton, Qt::NoModifier, titlePosition + QPoint(1, 0));
            if (dragWindow->position() != restoredPosition || dragWindow->size() != restoredSize) {
                qCritical() << "WINDOW_TITLE_TEST FAIL: small pointer motion must not move the window"; app.exit(2); return;
            }
            QTest::mouseDClick(dragWindow, Qt::LeftButton, Qt::NoModifier, titlePosition);
            QTest::qWait(150);
            if (titlePoint["width"].toDouble() < 20 || dragWindow->visibility() != QWindow::Maximized) {
                qCritical() << "WINDOW_TITLE_TEST FAIL: double click must maximize" << titlePoint << dragWindow->visibility(); app.exit(2); return;
            }
            QTest::mouseDClick(dragWindow, Qt::LeftButton, Qt::NoModifier, titlePosition);
            QTest::qWait(150);
            if (dragWindow->visibility() != QWindow::Windowed || dragWindow->size() != restoredSize) {
                qCritical() << "WINDOW_TITLE_TEST FAIL: double click must restore last size" << dragWindow->size() << restoredSize; app.exit(2); return;
            }
            qInfo() << "WINDOW_TITLE_TEST PASS: empty tab strip double click maximizes and restores last size";
            dragWindow->resize(1360, 900);
            if (surfaceTest) {
                QMetaObject::invokeMethod(root, "cleanFeaturePreview");
                const auto mapValue = [](const QVariant &value) {
                    return value.canConvert<QJSValue>() ? value.value<QJSValue>().toVariant().toMap() : value.toMap();
                };
                QTest::mouseMove(dragWindow, QPoint(80, 300));
                QTest::qWait(100);
                QVariant hintPoint;
                QMetaObject::invokeMethod(root, "surfaceHintPoint", Q_RETURN_ARG(QVariant, hintPoint));
                const auto point = mapValue(hintPoint);
                QTest::mouseMove(dragWindow, QPoint(point["x"].toInt(), point["y"].toInt()));
                QTest::qWait(850);
                QVariant hintVisible;
                QMetaObject::invokeMethod(root, "surfaceHintVisible", Q_RETURN_ARG(QVariant, hintVisible));
                if (!hintVisible.toBool()) { qCritical() << "SURFACE_TEST FAIL: glass tooltip not visible"; app.exit(2); return; }
                dragWindow->grabWindow().save("build/glass-tooltip.png");
                QTest::mouseMove(dragWindow, QPoint(80, 300));
                for (const QSize size : {QSize(1360,900), QSize(820,560)}) {
                    dragWindow->resize(size); QTest::qWait(100);
                    for (const QString &section : {QString("downloads"), QString("bookmarks"), QString("history"), QString("settings")}) {
                        QMetaObject::invokeMethod(root, "prepareSurfaceTest", Q_ARG(QVariant, section));
                        QTest::qWait(300);
                        QVariant state;
                        QMetaObject::invokeMethod(root, "surfaceTestState", Q_RETURN_ARG(QVariant, state));
                        const auto geometry = mapValue(state);
                        if (!geometry["visible"].toBool() || geometry["pageY"].toDouble() < 70
                            || qAbs(geometry["y"].toDouble()-geometry["pageY"].toDouble()) > 1
                            || qAbs(geometry["height"].toDouble()-geometry["pageHeight"].toDouble()) > 1
                            || geometry["modal"].toBool() || geometry["dim"].toBool()) {
                            qCritical() << "SURFACE_TEST FAIL: drawer workspace bounds" << section << size << geometry; app.exit(2); return;
                        }
                        dragWindow->grabWindow().save(QString("build/drawer-%1-%2.png").arg(section).arg(size.width()));
                    }
                }
                qInfo() << "SURFACE_TEST PASS: glass hover tooltip; downloads/bookmarks/history/settings bounded below chrome at 1360 and 820";
                app.exit(0); return;
            }
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
            auto terminalTransportChecks = std::make_shared<int>(0);
            auto retiredHttpProfile = std::make_shared<QPointer<QObject>>();
            QObject::connect(poll, &QTimer::timeout, &app, [root, &app, poll, attempts, privateProfileA, privateProfileB, controlBarTest, transportTest, trackerTest, controlStarted, retiredHttpProfile, terminalTransportChecks, &activeTransportStreams, &transportStreamsOpened, &transportStreamsClosed, &transportWsRequests, &transportTls] {
                QVariant state;
                if (*controlStarted) {
                    QMetaObject::invokeMethod(root, transportTest ? "pollTransportTest" : "pollControlBarTest", Q_RETURN_ARG(QVariant, state));
                    if (transportTest && !trackerTest) {
                        const auto value = root->property("transportTestProfile");
                        QObject *profile = value.value<QObject *>();
                        if (!profile) profile = value.value<QJSValue>().toQObject();
                        if (profile) *retiredHttpProfile = profile;
                    }
                    QVariant screenshot;
                    QMetaObject::invokeMethod(root, transportTest ? "takeTransportScreenshot" : "takeControlBarScreenshot", Q_RETURN_ARG(QVariant, screenshot));
                    if (!screenshot.toString().isEmpty()) {
                        auto window = qobject_cast<QQuickWindow *>(root);
                        window->grabWindow().save("build/" + screenshot.toString() + ".png");
                        qInfo() << "CONTROL_BAR_SNAPSHOT" << screenshot.toString() << "DPR" << window->devicePixelRatio();
                    }
                    QVariant input;
                    QMetaObject::invokeMethod(root, transportTest ? "takeTransportInput" : "takeControlBarInput", Q_RETURN_ARG(QVariant, input));
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
                    if (state.toInt() == 2 && (controlBarTest || transportTest)) {
                        QMetaObject::invokeMethod(root, transportTest ? "beginTransportTest" : "beginControlBarTest"); *controlStarted = true; state = 0;
                    }
                }
                if (state.toInt() == 1) return;
                if (state.toInt() == 2 && transportTest && !trackerTest) {
                    if (transportStreamsOpened != 3 || transportWsRequests != 0) {
                        qCritical() << "HTTP_CONTEXT_CONNECTION_TEST FAIL: stream controls or websocket escape" << transportStreamsOpened << transportWsRequests;
                        app.exit(2); return;
                    }
                    if (!activeTransportStreams.isEmpty() || transportStreamsClosed != 3 || !transportTls.activeSockets.isEmpty() || transportTls.closedSockets != 3) {
                        if (++*terminalTransportChecks < 10) return;
                        qCritical() << "HTTP_CONTEXT_CONNECTION_TEST FAIL: active stream survived context revoke" << activeTransportStreams.size() << transportStreamsClosed;
                        app.exit(2); return;
                    }
                    if (transportTls.handshakes != 3) {
                        qCritical() << "TRANSPORT_WSS_TEST FAIL: positive socket controls missing" << transportTls.handshakes;
                        app.exit(2); return;
                    }
                    if (*terminalTransportChecks != -1) {
                        qInfo() << "TRANSPORT_WSS_TEST PASS: three live WSS message controls; reload, popover and inline revoke closed all sockets and retired their profile owners";
                        qInfo() << "HTTP_CONTEXT_CONNECTION_TEST PASS: three real streams closed across reload and both revoke paths; temporary profiles denied all ws handshakes";
                        *terminalTransportChecks = -1;
                    }
                }
                const bool success = state.toInt() == 2 && privateProfileA.isNull() && privateProfileB.isNull() && (!transportTest || retiredHttpProfile->isNull());
                if (state.toInt() < 0 || ++*attempts > ((controlBarTest || transportTest) ? 700 : 200) || success) {
                    poll->stop();
                    qInfo() << "CYBOU_SMOKE_TEST" << (success ? "PASS" : "FAIL");
                    if (success) {
                        QMetaObject::invokeMethod(root, "prepareFeaturePreview");
                        QTimer::singleShot(500, &app, [root, &app, success] {
                            if (auto window = qobject_cast<QQuickWindow *>(root))
                                window->grabWindow().save("build/cybou-08-alpine-dawn.png");
                            QMetaObject::invokeMethod(root, "showCloudLauncherPreview");
                            QTimer::singleShot(400, &app, [root, &app, success] {
                                if (auto window = qobject_cast<QQuickWindow *>(root))
                                    window->grabWindow().save("build/cybou-08-cloud-launcher.png");
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
