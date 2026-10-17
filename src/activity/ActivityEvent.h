#pragma once
#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QVariantMap>

enum class ActivityScope {
    Tab,
    Profile,
    Unknown
};

enum class ActivityKind {
    PermissionRequest,
    PermissionDecision,
    PopupAttempt,
    NetworkFilter,
    TrackerBlocked,
    TrackerException
};

struct ActivityEvent {
    ActivityScope scope = ActivityScope::Unknown;
    ActivityKind kind = ActivityKind::PermissionRequest;
    QString key;             // "camera", "mic", "popups", "network", etc.
    QString tabId;           // Valid when scope == ActivityScope::Tab
    QString topOrigin;       // Sanitized top origin (e.g. "https://example.com")
    QString requesterOrigin; // Sanitized requester origin
    qint64 timestamp = 0;    // Milliseconds since epoch
    bool pending = false;
    bool blocked = false;
    bool allowed = false;
    quint64 requestId = 0;
    QString category;

    QVariantMap toMap() const {
        QVariantMap map;
        map[QStringLiteral("scope")] = scope == ActivityScope::Tab ? QStringLiteral("tab")
                                     : scope == ActivityScope::Profile ? QStringLiteral("profile")
                                     : QStringLiteral("unknown");
        map[QStringLiteral("key")] = key;
        map[QStringLiteral("tab")] = tabId;
        map[QStringLiteral("top")] = topOrigin;
        map[QStringLiteral("origin")] = requesterOrigin;
        map[QStringLiteral("requester")] = requesterOrigin;
        map[QStringLiteral("time")] = timestamp;
        map[QStringLiteral("pending")] = pending;
        map[QStringLiteral("blocked")] = blocked;
        map[QStringLiteral("allowed")] = allowed;
        map[QStringLiteral("requestId")] = requestId;
        if (!category.isEmpty()) map[QStringLiteral("category")] = category;
        return map;
    }
};
