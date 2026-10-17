#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QIcon>
#include <QTimer>
#include <QQuickWindow>
#include <QTranslator>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include "browserstore.h"

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    app.setApplicationName("CYBOU");
    app.setOrganizationName("CYBOU");
    app.setApplicationVersion(QStringLiteral(CYBOU_PRODUCT_VERSION));
    app.setWindowIcon(QIcon(":/assets/cybou.png"));
    QQuickStyle::setStyle("Basic");

    BrowserStore store;

    QTranslator controlTranslator;
    QQmlApplicationEngine engine;
    auto translateControlBar = [&] {
        app.removeTranslator(&controlTranslator);
        if (store.controlBarLanguage() == "ru" && controlTranslator.load(":/translations/cybou_ru.qm"))
            app.installTranslator(&controlTranslator);
        engine.retranslate();
    };
    QObject::connect(&store, &BrowserStore::languageChanged, &engine, translateControlBar);
    translateControlBar();

    engine.rootContext()->setContextProperty("browserStore", &store);
    engine.rootContext()->setContextProperty("smokeMode", false);
    engine.rootContext()->setContextProperty("trackerTestMode", false);
    engine.rootContext()->setContextProperty("smokeBaseUrl", QString());
    engine.rootContext()->setContextProperty("smokeDownloadPath", QString());

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
    if (app.arguments().contains("--open-library")) initialProps["initialLibrarySection"] = "downloads";
    if (app.arguments().contains("--open-settings")) initialProps["initialSettingsOpen"] = true;
    if (app.arguments().contains("--open-menu")) initialProps["initialMenuOpen"] = true;
    if (app.arguments().contains("--open-cloud")) initialProps["initialCloudLauncherOpen"] = true;
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

    return app.exec();
}
