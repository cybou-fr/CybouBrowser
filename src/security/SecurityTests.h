#pragma once
#include <QGuiApplication>
#include <QTimer>
#include <QDebug>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "../controlbar/RequestObserver.h"
#include "DownloadPolicy.h"
#include "../browserstore.h"
#include <QTemporaryDir>
#include <QFile>

inline int runSecurityTests(QGuiApplication &app, quint16 port) {
    for (const auto &name : QStringList{"tool.exe", "image.png.exe", "setup.msi", "script.ps1", "script.js", "script.bat", "link.lnk", "link.url", "app.desktop", "page.html", "drawing.svg", "macro.docm", "archive.zip", "unknown", "image.png.", "image.png ", "image.png:payload.exe"}) {
        if (DownloadPolicy::allowsExternalOpen(name)) { qCritical() << "DOWNLOAD_POLICY_TEST FAIL" << name; return 2; }
    }
    for (const auto &name : QStringList{"report.PDF", "notes.txt", "photo.jpg", "table.xlsx"}) {
        if (!DownloadPolicy::allowsExternalOpen(name)) { qCritical() << "DOWNLOAD_POLICY_TEST FAIL" << name; return 2; }
    }
    qInfo() << "DOWNLOAD_POLICY_TEST PASS";
    {
        QTemporaryDir files;
        if (!files.isValid()) return 2;
        const auto blockedPath = files.filePath("document.pdf.exe");
        const auto documentPath = files.filePath("document.pdf");
        for (const auto &path : {blockedPath, documentPath}) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly) || file.write("test") != 4) return 2;
        }
        BrowserStore store;
        if (store.canOpenDownloadedFile(blockedPath) || store.openDownloadedFile(blockedPath)
            || !store.canOpenDownloadedFile(documentPath)
            || store.canOpenDownloadedFile(files.filePath("missing.pdf"))) return 2;
        qInfo() << "DOWNLOAD_LAUNCH_GUARD_TEST PASS: executable and missing file denied, document eligible for review";
    }
    const struct { const char *url; bool blocked; } cases[] = {
        {"http://example.org/", true}, {"HTTP://example.org/", true},
        {"http://127.0.0.1/", true}, {"http://[::1]/", true},
        {"http://localhost/", true}, {"ws://example.org/socket", true},
        {"https://example.org/", false}, {"wss://example.org/socket", false},
        {"file:///test.html", false}, {"about:blank", false}
    };
    for (const auto &test : cases) {
        if (NetworkPolicy::blocks(QUrl(test.url), true) != test.blocked
            || NetworkPolicy::blocks(QUrl(test.url), false)) return 2;
    }
    if (!NetworkPolicy::hasWebCredentials(QUrl("https://user:secret@example.org/"))
        || NetworkPolicy::hasWebCredentials(QUrl("https://example.org/?email=user@example.org"))) return 2;

    QWebEngineProfile profile;
    RequestObserver interceptor(&profile);
    profile.setUrlRequestInterceptor(&interceptor);
    QWebEnginePage page(&profile);
    int step = 0;
    interceptor.setHttpsOnly(true);
    QObject::connect(&page, &QWebEnginePage::titleChanged, &app, [&](const QString &title) {
        if (step != 2) return;
        if (title == "subresource-escaped") { qCritical() << "SECURITY_SUBRESOURCE_TEST FAIL"; app.exit(2); }
        if (title != "subresource-denied") return;
        qInfo() << "SECURITY_SUBRESOURCE_TEST PASS: existing page fetch blocked after preference change";
        QTimer::singleShot(0, &page, [&] {
            interceptor.setHttpsOnly(false);
            page.load(QUrl(QString("http://user:secret@127.0.0.1:%1/page").arg(port)));
        });
    });
    QObject::connect(&page, &QWebEnginePage::loadFinished, &app, [&](bool ok) {
        const bool expected = step == 1;
        if (ok != expected) { qCritical() << "SECURITY_NETWORK_TEST FAIL" << step << ok; app.exit(2); return; }
        ++step;
        if (step == 3) { qInfo() << "SECURITY_NETWORK_TEST PASS: strict HTTP denied, explicit opt-out loaded, URL credentials denied"; app.exit(0); return; }
        if (step == 2) {
            interceptor.setHttpsOnly(true);
            page.runJavaScript("fetch('/security-probe').then(()=>document.title='subresource-escaped').catch(()=>document.title='subresource-denied')");
            return;
        }
        QTimer::singleShot(0, &page, [&] {
            interceptor.setHttpsOnly(false);
            const auto host = step == 2 ? "user:secret@127.0.0.1" : "127.0.0.1";
            page.load(QUrl(QString("http://%1:%2/page?step=%3").arg(host).arg(port).arg(step)));
        });
    });
    QTimer::singleShot(20000, &app, [&] { qCritical() << "SECURITY_NETWORK_TEST TIMEOUT"; app.exit(2); });
    page.load(QUrl(QString("http://127.0.0.1:%1/page").arg(port)));
    return app.exec();
}
