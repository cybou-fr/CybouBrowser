#pragma once
#include <QObject>
#include <QSettings>
#include <QVariantMap>

// What Windows itself records about camera and microphone use (the data behind the privacy indicator and
// Settings > Privacy > Camera "recent activity"): per executable, LastUsedTimeStart/Stop as FILETIME; Stop is 0 while
// the device is in use. Read-only; used by cybou_tests --media-device-test to check a real device against the OS view.
class OsCaptureProbe : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    // capability: "webcam" or "microphone". Entries whose executable path contains `match` (case-insensitive).
    Q_INVOKABLE QVariantMap state(const QString &capability, const QString &match = QStringLiteral("qtwebengineprocess")) const {
        QSettings store(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\")
                        + capability + QStringLiteral("\\NonPackaged"), QSettings::NativeFormat);
        QVariantList entries;
        bool inUse = false;
        for (const auto &key : store.childGroups()) {
            if (!key.contains(match, Qt::CaseInsensitive)) continue;
            store.beginGroup(key);
            const auto start = store.value(QStringLiteral("LastUsedTimeStart")).toULongLong();
            const auto stop = store.value(QStringLiteral("LastUsedTimeStop")).toULongLong();
            store.endGroup();
            inUse |= start != 0 && stop == 0;
            entries.append(QVariantMap{{"exe", QString(key).replace('#', '\\')}, {"start", QString::number(start)}, {"stop", QString::number(stop)}});
        }
        return {{"inUse", inUse}, {"entries", entries}};
    }
};
