#pragma once
#include <QCoreApplication>
#include <QSettings>
#include <QStringList>

// The browser's persistent store (history, session, bookmarks, preferences).
//
// It used to share the default QSettings location (HKCU\Software\<org>\<app>) with another
// application that uses the same organization and application name; that application's own
// values live in the same key, so a "reset settings" there would erase browser data and the
// browser's history was readable by it. The browser now writes to "<app> Browser".
class BrowserSettings final : public QSettings {
public:
    BrowserSettings()
        : QSettings(QSettings::NativeFormat, QSettings::UserScope, QCoreApplication::organizationName(),
                    QCoreApplication::applicationName() + QStringLiteral(" Browser")) {}

    // Exact keys the browser has ever written. Anything else in the shared key belongs to someone
    // else and is never copied or removed.
    static const QStringList &ownedKeys() {
        static const QStringList keys{
            "history", "session", "bookmarks", "cloud/configuration",
            "preferences/askDownloadLocation", "preferences/blockThirdPartyCookies", "preferences/blockTrackers",
            "preferences/customWallpaperPath", "preferences/disableWasm", "preferences/downloadsPath", "preferences/glassBlurLevel",
            "preferences/httpsFirst", "preferences/restoreSession", "preferences/searchEngine",
            "preferences/strictWebRtc", "preferences/wallpaperType",
            "ui/controlBar/disableGlow", "ui/controlBar/language", "ui/controlBar/position", "ui/controlBar/reduceMotion",
            "privacy/storedUrlsSanitized1", "ui/strictWebRtcNoticeShown"};
        return keys;
    }
    // Keys the browser wrote in the past and no longer reads; they are deleted, not copied.
    static const QStringList &retiredKeys() {
        static const QStringList keys{"controlPolicies", "preferences/httpsOnly"};
        return keys;
    }

    // Moves the browser's keys from the shared store to the dedicated one. Values are copied first,
    // compared, and only then removed from the shared store; on any mismatch nothing is removed and
    // the step is retried at the next start. Returns true when it ran to completion.
    static bool migrate(QSettings &shared, QSettings &target) {
        if (target.value("storage/migratedFromSharedKey1", false).toBool()) return false;
        QStringList moved, copied;
        for (const QString &key : ownedKeys()) {
            if (!shared.contains(key)) continue;
            if (!target.contains(key)) { target.setValue(key, shared.value(key)); copied << key; } // never overwrite newer data
            moved << key;
        }
        target.sync();
        bool verified = target.status() == QSettings::NoError;
        for (const QString &key : moved) {
            if (!target.contains(key)) verified = false;
            if (copied.contains(key) && target.value(key) != shared.value(key)) verified = false;
        }
        if (!verified) return false;
        for (const QString &key : moved) shared.remove(key);
        for (const QString &key : retiredKeys()) shared.remove(key);
        shared.sync();
        target.setValue("storage/migratedFromSharedKey1", true);
        target.sync();
        return true;
    }

    // Uses the default (shared) location as the source; call once at start-up, after the
    // organization and application names are set and before any other settings access.
    static void migrate() {
        QSettings shared;
        BrowserSettings target;
        migrate(shared, target);
    }
};
