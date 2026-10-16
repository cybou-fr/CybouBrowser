#pragma once
#include "SiteContext.h"
#include <QDateTime>
#include <QDeadlineTimer>
#include <QStringList>
#include <algorithm>

// Both clocks bound the grant: a wall-clock jump backwards cannot extend it.
struct PermissionDeadline {
    explicit PermissionDeadline(qint64 durationMs = 0)
        : monotonic(durationMs, Qt::PreciseTimer), wallExpiresAt(QDateTime::currentMSecsSinceEpoch() + durationMs) {}
    qint64 remainingTime() const {
        return std::max<qint64>(0, std::min(monotonic.remainingTime(), wallExpiresAt - QDateTime::currentMSecsSinceEpoch()));
    }
    bool expired() const { return remainingTime() == 0; }
    QDeadlineTimer monotonic;
    qint64 wallExpiresAt;
};

struct PermissionLease {
    quint64 requestId;
    SiteContext context;
    QStringList keys;
};

