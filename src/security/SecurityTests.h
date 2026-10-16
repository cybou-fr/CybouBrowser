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
    const struct { const char *url; const char *host; } addresses[] = {
        {"https://a.b.example.org/long/path?q=full", "a.b.example.org"},
        {"http://user:secret@example.org:8080/path", "example.org:8080"},
        {"https://example.org:443/", "example.org"},
        {"http://localhost:8000/", "localhost:8000"},
        {"https://[2001:db8::1]:8443/path", "[2001:db8::1]:8443"},
        {"https://xn--bcher-kva.example/", "xn--bcher-kva.example"},
        {"about:blank", ""}, {"file:///C:/docs/report.pdf", "file: report.pdf"},
        {"data:text/html,hello", "data:"}
    };
    for (const auto &test : addresses) {
        const QUrl url(QString::fromUtf8(test.url)); const auto raw = url.toString();
        if (AddressDisplay::host(url) != QString::fromUtf8(test.host) || url.toString() != raw) {
            qCritical() << "ADDRESS_DISPLAY_TEST FAIL" << test.url << AddressDisplay::host(url); return 2;
        }
    }
    const QUrl unicode(QString::fromUtf8("https://b\xC3\xBC" "cher.example/path?secret=1"));
    if (AddressDisplay::host(unicode) != "xn--bcher-kva.example") return 2;
    qInfo() << "ADDRESS_DISPLAY_TEST PASS: exact host, port, IPv6, ACE/IDN, file and blank";
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

    {
        using namespace TrackerBlocker;
        if (!isTracker(QUrl("https://www.google-analytics.com/analytics.js"), QUrl("https://example.com"))
            || !isTracker(QUrl("https://adservice.google.com/adsid/integrator.js"), QUrl("https://news.com"))
            || !isTracker(QUrl("https://connect.facebook.net/en_US/fbevents.js"), QUrl("https://shop.com"))
            || !isTracker(QUrl("https://mc.yandex.ru/metrika/tag.js"), QUrl("https://blog.com"))
            || isTracker(QUrl("https://www.google-analytics.com/dashboard"), QUrl("https://www.google-analytics.com/"))
            || isTracker(QUrl("https://example.com/script.js"), QUrl("https://example.com/"))) {
            qCritical() << "TRACKER_BLOCKER_TEST FAIL"; return 2;
        }
        qInfo() << "TRACKER_BLOCKER_TEST PASS: third-party analytics and ad networks blocked, first-party preserved";
    }

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
