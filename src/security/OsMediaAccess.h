#pragma once
#include <QSettings>
#include <QString>
#include <QStringList>
#include <optional>

// Whether Windows lets desktop applications use the camera or the microphone at all (Settings > Privacy & security).
// When it does not, the engine still asks CYBOU and then fails to open the device ("Permission denied by system",
// observed 2026-10-29), so a grant would be shown in the Control Bar while nothing can use it. CYBOU therefore refuses
// such a request before any prompt and creates no grant. Read only: CYBOU never changes these settings.
struct OsMediaAccess {
    // cybou_tests sets this to exercise the refusal without touching the real system setting.
    static std::optional<bool> &testOverride() { static std::optional<bool> value; return value; }

    static bool allowed(const QString &key) {
        const QString capability = key == QStringLiteral("camera") ? QStringLiteral("webcam")
                                 : key == QStringLiteral("mic") ? QStringLiteral("microphone") : QString();
        if (capability.isEmpty()) return true;
        if (testOverride()) return *testOverride();
#ifdef Q_OS_WIN
        // The device-wide switch, the per-user switch and the desktop-apps switch must all allow it.
        const QString store = QStringLiteral("\\Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\") + capability;
        for (const QString &path : {QStringLiteral("HKEY_LOCAL_MACHINE") + store, QStringLiteral("HKEY_CURRENT_USER") + store,
                                    QStringLiteral("HKEY_CURRENT_USER") + store + QStringLiteral("\\NonPackaged")}) {
            QSettings settings(path, QSettings::NativeFormat);
            if (settings.value(QStringLiteral("Value")).toString().compare(QStringLiteral("Deny"), Qt::CaseInsensitive) == 0) return false;
        }
#endif
        return true;
    }
    static bool allowedAll(const QStringList &keys) {
        for (const auto &key : keys) if (!allowed(key)) return false;
        return true;
    }
};
