#pragma once
#include <QGuiApplication>
#include <QTimer>
#include <QDebug>
#include <QtWebEngineCore/QWebEnginePage>
#include <QtWebEngineCore/QWebEngineProfile>
#include "../controlbar/RequestObserver.h"
#include "DownloadPolicy.h"
#include "StoredUrl.h"
#include "BrowserSettings.h"
#include <QSettings>
#include "NetworkBoundaryTests.h"
#include "TlsBoundaryTests.h"
#include "HttpRedirectTests.h"
#include "PermissionTests.h"
#include "HttpLeaseTests.h"
#include "TrackerLeaseTests.h"
#include "NavigationGuardTests.h"
#include "../activity/ActivityTests.h"
#include "../cloud/CloudCoreTests.h"
#include "../browserstore.h"
#include <QTemporaryDir>
#include <QFile>

inline int runSecurityTests(QGuiApplication &app, quint16 port) {
    if (!runTrackerLeaseTests()) { qCritical() << "TRACKER_LEASE_TEST FAIL"; return 2; }
    if (!runHttpLeaseTests()) { qCritical() << "HTTP_LEASE_TEST FAIL"; return 2; }
    if (!runPermissionContextTests()) { qCritical() << "PERMISSION_CONTEXT_TEST FAIL"; return 2; }
    if (!runActivityAggregatorTests()) { qCritical() << "ACTIVITY_AGGREGATOR_TEST FAIL"; return 2; }
    if (!runNavigationGuardTests()) { qCritical() << "NAVIGATION_GUARD_TEST FAIL"; return 2; }
    if (!runCloudCoreTests()) { qCritical() << "CLOUD_CORE_TEST FAIL"; return 2; }
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
    for (const auto &name : QStringList{"CON.txt", "con.PDF", "NUL.png", "prn.docx", "aux", "com1.txt", "LPT9.pdf", "COM\u00b9.txt", "CONIN$.txt", "CONOUT$.pdf", "con .txt"}) {
        if (DownloadPolicy::allowsExternalOpen(name)) { qCritical() << "DOWNLOAD_POLICY_TEST FAIL reserved device name" << name; return 2; }
    }
    for (const auto &name : QStringList{"console.txt", "company.pdf", "com10.txt", "lpt0.txt", "a.con.txt", "icon.png"}) {
        if (!DownloadPolicy::allowsExternalOpen(name)) { qCritical() << "DOWNLOAD_POLICY_TEST FAIL false positive" << name; return 2; }
    }
    qInfo() << "DOWNLOAD_POLICY_TEST PASS";
    {
        // Migration of the browser's keys out of a store shared with another application.
        QTemporaryDir dir;
        if (!dir.isValid()) return 2;
        QSettings shared(dir.filePath("shared.ini"), QSettings::IniFormat), target(dir.filePath("target.ini"), QSettings::IniFormat);
        const QVariantList history{QVariantMap{{"url", "https://example.org/a"}, {"title", "A"}}};
        shared.setValue("MainWindowGeometry", QByteArray("geometry-bytes"));
        shared.setValue("strDataDir", "C:/node-data");
        shared.setValue("ui/otherAppSetting", 7);
        shared.setValue("privacy/otherAppSetting", 8);
        shared.setValue("preferences/foreignGroupMember", 9);
        shared.setValue("history", history);
        shared.setValue("session", QVariantList{QVariantMap{{"url", "https://example.org/s"}, {"pinned", true}}});
        shared.setValue("bookmarks", history);
        shared.setValue("cloud/configuration", "{\"v\":1}");
        shared.setValue("preferences/searchEngine", "Brave");
        shared.setValue("ui/controlBar/language", "ru");
        shared.setValue("privacy/storedUrlsSanitized1", true);
        shared.setValue("controlPolicies", "retired");
        shared.sync();
        target.setValue("preferences/searchEngine", "DuckDuckGo"); // newer data in the target must win
        if (!BrowserSettings::migrate(shared, target)) { qCritical() << "BROWSER_SETTINGS_TEST FAIL: migration did not complete"; return 2; }
        for (const char *key : {"history", "session", "bookmarks", "cloud/configuration", "ui/controlBar/language", "privacy/storedUrlsSanitized1"}) {
            if (!target.contains(key) || shared.contains(key)) { qCritical() << "BROWSER_SETTINGS_TEST FAIL: key not moved" << key; return 2; }
        }
        if (target.value("history") != history || target.value("ui/controlBar/language") != "ru") { qCritical() << "BROWSER_SETTINGS_TEST FAIL: values changed in transit"; return 2; }
        if (target.value("preferences/searchEngine") != "DuckDuckGo" || shared.contains("preferences/searchEngine")) { qCritical() << "BROWSER_SETTINGS_TEST FAIL: newer target value overwritten or old copy left"; return 2; }
        if (shared.contains("controlPolicies")) { qCritical() << "BROWSER_SETTINGS_TEST FAIL: retired key kept"; return 2; }
        for (const char *key : {"MainWindowGeometry", "strDataDir", "ui/otherAppSetting", "privacy/otherAppSetting", "preferences/foreignGroupMember"}) {
            if (!shared.contains(key) || target.contains(key)) { qCritical() << "BROWSER_SETTINGS_TEST FAIL: foreign value touched" << key; return 2; }
        }
        shared.setValue("history", QVariantList{});
        if (BrowserSettings::migrate(shared, target) || !shared.contains("history")) { qCritical() << "BROWSER_SETTINGS_TEST FAIL: migration is not one-shot"; return 2; }
        qInfo() << "BROWSER_SETTINGS_TEST PASS: browser keys moved with values intact, newer data kept, retired keys dropped, foreign values untouched, one-shot";
    }
    {
        // Secrets that sites put in URLs must not reach the persistent history and bookmark stores.
        struct { const char *input, *expected; } urls[] = {
            {"https://example.org/a?b=1&c=2", "https://example.org/a?b=1&c=2"},              // unchanged, same string
            {"https://example.org/cb#access_token=SECRET&state=x", "https://example.org/cb"},
            {"https://u:pw@example.org/p", "https://example.org/p"},
            {"https://example.org/reset?token=SECRET&lang=fr", "https://example.org/reset?token=redacted&lang=fr"},
            {"https://example.org/o?Access-Token=SECRET&code=SECRET2&x=1", "https://example.org/o?Access-Token=redacted&code=redacted&x=1"},
            {"https://example.org/s?X-Amz-Signature=SECRET&X-Amz-Credential=C", "https://example.org/s?X-Amz-Signature=redacted&X-Amz-Credential=redacted"},
            {"https://example.org/z?zipcode=75001&encoding=utf8&monkey=1", "https://example.org/z?zipcode=75001&encoding=utf8&monkey=1"},
            {"https://example.org/q?api_key=SECRET#frag", "https://example.org/q?api_key=redacted"},
            {"https://example.org/a?x=1;token=SECRET", "https://example.org/a?x=1;token=redacted"},
            {"https://example.org/r?continue=https%3A%2F%2Fapp%2Fcb%3Ftoken%3DSECRET&x=1", "https://example.org/r?continue=redacted&x=1"},
            {"https://example.org/r?next=/a?access_token=SECRET&x=1", "https://example.org/r?next=redacted&x=1"},
            {"https://example.org/r?back=%2Fhome&x=1", "https://example.org/r?back=%2Fhome&x=1"},
        };
        for (const auto &u : urls) {
            const QString got = StoredUrl::sanitize(QString::fromUtf8(u.input));
            if (got != QString::fromUtf8(u.expected) || got.contains("SECRET")) { qCritical() << "STORED_URL_TEST FAIL" << u.input << "->" << got; return 2; }
        }
        BrowserStore store;
        const QString leaky = "https://example.org/cb?access_token=SECRETVALUE&page=2#id_token=SECRETFRAG";
        store.clearHistory();
        store.recordVisit(leaky, QString());
        store.recordVisit(leaky, leaky); // title equal to the URL must not smuggle it back
        const auto history = store.history();
        if (history.size() != 1) { qCritical() << "STORED_URL_TEST FAIL: history entries" << history.size(); return 2; }
        const auto row = history.first().toMap();
        if (row.value("url").toString().contains("SECRET") || row.value("title").toString().contains("SECRET")) { qCritical() << "STORED_URL_TEST FAIL: history leaks" << row; return 2; }
        const bool wasBookmarked = store.isBookmarked(leaky);
        if (!wasBookmarked) store.toggleBookmark(leaky, leaky);
        if (!store.isBookmarked(leaky)) { qCritical() << "STORED_URL_TEST FAIL: bookmark lookup by raw URL"; return 2; }
        for (const auto &item : store.bookmarks()) {
            const auto mark = item.toMap();
            if (mark.value("url").toString().contains("SECRETVALUE") || mark.value("title").toString().contains("SECRET")) { qCritical() << "STORED_URL_TEST FAIL: bookmark leaks" << mark; return 2; }
        }
        if (!wasBookmarked) store.toggleBookmark(leaky, leaky);
        store.clearHistory();
        // Migration: entries written by earlier versions are cleaned once.
        BrowserSettings legacy;
        const QVariant keepHistory = legacy.value("history"), keepBookmarks = legacy.value("bookmarks");
        legacy.setValue("history", QVariantList{QVariantMap{{"url", leaky}, {"title", leaky}, {"time", "2026-01-01T00:00:00"}}});
        legacy.setValue("bookmarks", QVariantList{QVariantMap{{"url", leaky}, {"title", "kept title"}}});
        legacy.remove("privacy/storedUrlsSanitized1"); legacy.sync();
        {
            BrowserStore migrated;
            const auto h = migrated.history().first().toMap(), b = migrated.bookmarks().first().toMap();
            if (h.value("url").toString().contains("SECRET") || h.value("title").toString().contains("SECRET") || b.value("url").toString().contains("SECRET") || b.value("title").toString() != "kept title") {
                qCritical() << "STORED_URL_TEST FAIL: migration" << h << b; return 2;
            }
        }
        legacy.setValue("history", keepHistory); legacy.setValue("bookmarks", keepBookmarks); legacy.sync();
        qInfo() << "STORED_URL_TEST PASS: fragments, credentials and secret query values never reach history or bookmarks; ordinary URLs unchanged; legacy entries migrated once";
    }
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
        if (!store.isPrivateNetwork(QUrl("http://127.0.0.1:8080/"))
            || !store.isPrivateNetwork(QUrl("http://127.0.0.5:80/"))
            || !store.isPrivateNetwork(QUrl("http://localhost:3000/"))
            || !store.isPrivateNetwork(QUrl("http://10.1.2.3/"))
            || !store.isPrivateNetwork(QUrl("http://172.16.5.6/"))
            || !store.isPrivateNetwork(QUrl("http://192.168.1.1/"))
            || !store.isPrivateNetwork(QUrl("http://169.254.10.20/"))
            || store.isPrivateNetwork(QUrl("https://8.8.8.8/"))
            || store.isPrivateNetwork(QUrl("https://example.com/"))) return 2;
        qInfo() << "PRIVATE_NETWORK_GUARD_TEST PASS: loopback, RFC 1918 and link-local classified as private";
        if (!store.requiresRestrictedBrowsing(QUrl("https://xn--pple-43d.example/"))
            || !store.requiresRestrictedBrowsing(QUrl("http://example.com:22/"))
            || !store.requiresRestrictedBrowsing(QUrl("http://example.com:6379/"))
            || store.requiresRestrictedBrowsing(QUrl("https://example.com/"))
            || store.requiresRestrictedBrowsing(QUrl("http://example.com:8080/"))) return 2;
        if (store.restrictedBrowsingReason(QUrl("https://xn--pple-43d.example/")).isEmpty()
            || store.restrictedBrowsingReason(QUrl("http://example.com:22/")).isEmpty()
            || !store.restrictedBrowsingReason(QUrl("https://example.com/")).isEmpty()) return 2;
        qInfo() << "RESTRICTED_BROWSING_GUARD_TEST PASS: homographs and dangerous ports flagged with reason";
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

    {
        BrowserStore store;
        const QUrl publicHttp("http://example.org/path?q=1");
        const QUrl publicHttps("https://example.org/path?q=1");
        const QUrl localHttp("http://localhost:8080/test");
        const QUrl ipHttp("http://127.0.0.1:3000/");
        if (!store.canRequestTemporaryTrackers(publicHttps) || store.canRequestTemporaryTrackers(publicHttp)
            || store.canRequestTemporaryTrackers(ipHttp) || store.canRequestTemporaryTrackers(QUrl("https://user:secret@example.org/"))) {
            qCritical() << "TRACKER_PROFILE_POLICY_TEST FAIL"; return 2;
        }
        qInfo() << "TRACKER_PROFILE_POLICY_TEST PASS: production factory accepts HTTPS only, including denial of loopback HTTP and credentials";

        if (!store.httpsFirst()) { qCritical() << "HTTPS_FIRST_TEST: httpsFirst not default true"; return 2; }
        if (!store.shouldUpgradeToHttps(publicHttp)) { qCritical() << "HTTPS_FIRST_TEST: shouldUpgradeToHttps failed for publicHttp"; return 2; }
        if (store.shouldUpgradeToHttps(publicHttps)) { qCritical() << "HTTPS_FIRST_TEST: shouldUpgradeToHttps upgraded publicHttps"; return 2; }
        if (store.shouldUpgradeToHttps(localHttp)) { qCritical() << "HTTPS_FIRST_TEST: shouldUpgradeToHttps upgraded localHttp"; return 2; }
        if (store.shouldUpgradeToHttps(ipHttp)) { qCritical() << "HTTPS_FIRST_TEST: shouldUpgradeToHttps upgraded ipHttp"; return 2; }

        const QUrl upgraded = store.upgradeToHttps(publicHttp);
        if (upgraded != QUrl("https://example.org/path?q=1")) { qCritical() << "HTTPS_FIRST_TEST: upgradeToHttps mismatch" << upgraded; return 2; }

        if (store.navigationBlockReason(publicHttp).isEmpty()) return 2;
        qInfo() << "HTTPS_FIRST_TEST PASS: upgrade retained, public HTTP denied, no host fallback";
    }

    if (!runNetworkBoundaryTests() || !runTlsBoundaryTests() || !runTlsCertificateMatrixTests() || !runHttpRedirectTests() || !runHttpsDowngradeRedirectTests()) return 2;

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
        if (step == 0) {
            const auto events = interceptor.sample();
            if (events.isEmpty() || !events.last().toMap().value("blocked").toBool()
                || events.last().toMap().value("reason").toString() != "insecure-transport") {
                qCritical() << "BLOCKED_NETWORK_EVENT_TEST FAIL"; app.exit(2); return;
            }
            qInfo() << "BLOCKED_NETWORK_EVENT_TEST PASS: strict deny recorded with reason and profile scope";
        }
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
