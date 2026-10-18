#pragma once
#include <memory>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include <QtWebEngineCore/QWebEngineProfileBuilder>

// Does the engine keep its own copy of browsing history in the profile directory, and which
// engine call (if any) removes it? Visits a few marker URLs in a persistent temporary profile,
// optionally applies an action, destroys the profile (flushing databases) and scans every file in
// the storage directory for the marker bytes. Diagnostic: it reports observations.
inline QStringList scanForMarker(const QString &root, const QByteArray &marker) {
    QStringList found;
    QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) { found << "(unreadable) " + QDir(root).relativeFilePath(path); continue; }
        if (file.readAll().contains(marker)) found << QDir(root).relativeFilePath(path);
    }
    found.sort();
    return found;
}

inline int runEngineHistoryProbe() {
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
    qCritical() << "ENGINE_HISTORY_PROBE BLOCKED: needs Qt 6.10+ builder API"; return 2;
#else
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) return 2;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server] {
        while (auto socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                socket->readAll();
                const QByteArray body("<!doctype html><title>Marker page</title><link rel=icon href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR4nGP4z8DwHwAFAAH/q842iQAAAABJRU5ErkJggg==\"><p>x");
                socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        }
    });
    const QByteArray marker = "engine-history-marker";
    struct Scenario { const char *label; int action; }; // 0 = nothing, 1 = clearAllVisitedLinks, 2 = clearHttpCache (the app's clear-cache action)
    for (const Scenario scenario : {Scenario{"app-history-clear-no-engine-call", 0}, Scenario{"clearAllVisitedLinks", 1}, Scenario{"clearHttpCache", 2}}) {
        QTemporaryDir dir;
        if (!dir.isValid()) return 2;
        {
            QWebEngineProfileBuilder builder;
            builder.setPersistentStoragePath(dir.filePath("data"));
            builder.setCachePath(dir.filePath("cache"));
            std::unique_ptr<QWebEngineProfile> profile(builder.createProfile(QStringLiteral("HistoryProbe")));
            if (!profile || profile->isOffTheRecord()) { qCritical() << "ENGINE_HISTORY_PROBE FAIL: persistent profile"; return 2; }
            for (int i = 0; i < 3; ++i) {
                QWebEnginePage page(profile.get());
                QEventLoop loop; QTimer timeout; timeout.setSingleShot(true);
                QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
                QObject::connect(&page, &QWebEnginePage::loadFinished, &loop, &QEventLoop::quit);
                timeout.start(10000);
                page.load(QUrl(QString("http://127.0.0.1:%1/%2-%3?q=1").arg(server.serverPort()).arg(QString::fromLatin1(marker)).arg(i)));
                loop.exec();
                QEventLoop settle; QTimer::singleShot(1200, &settle, &QEventLoop::quit); settle.exec();
            }
            if (scenario.action == 1) profile->clearAllVisitedLinks();
            if (scenario.action == 2) { QEventLoop cleared; QObject::connect(profile.get(), &QWebEngineProfile::clearHttpCacheCompleted, &cleared, &QEventLoop::quit); QTimer::singleShot(8000, &cleared, &QEventLoop::quit); profile->clearHttpCache(); cleared.exec(); }
            // Chromium commits history in batches; wait past its commit interval before closing.
            QEventLoop settle; QTimer::singleShot(15000, &settle, &QEventLoop::quit); settle.exec();
        } // profile destroyed: databases flushed
        QEventLoop closeWait; QTimer::singleShot(1500, &closeWait, &QEventLoop::quit); closeWait.exec();
        const auto hits = scanForMarker(dir.path(), marker);
        if (scenario.action == 0) { QStringList all; QDirIterator list(dir.path(), QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories); while (list.hasNext()) { const QString f = list.next(); all << QDir(dir.path()).relativeFilePath(f) + ":" + QString::number(QFileInfo(f).size()); } qInfo().noquote() << "ENGINE_HISTORY_FILES" << all.join(" "); }
        qInfo().noquote() << "ENGINE_HISTORY_PROBE" << scenario.label << "filesContainingVisitedUrl" << hits.size() << hits.join(" | ");
    }
    return 0;
#endif
}
