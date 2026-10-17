#include "ActivityAggregator.h"
#include <QDateTime>

ActivityAggregator::ActivityAggregator(qsizetype maxEvents)
    : m_maxEvents(maxEvents) {}

void ActivityAggregator::recordPermission(const QString &key, const QString &tab, const QString &top,
                                         const QString &requester, bool pending, bool blocked, quint64 requestId) {
    QMutexLocker locker(&m_mutex);
    ActivityEvent event;
    event.scope = ActivityScope::Tab;
    event.kind = ActivityKind::PermissionRequest;
    event.key = key;
    event.tabId = tab;
    event.topOrigin = top;
    event.requesterOrigin = requester;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.pending = pending;
    event.blocked = blocked;
    event.requestId = requestId;

    m_events.append(event);
    if (m_events.size() > m_maxEvents) {
        m_events.removeFirst();
    }
}

void ActivityAggregator::finishPermission(const QString &key, const QString &tab, const QString &requester,
                                         bool allowed, quint64 requestId) {
    QMutexLocker locker(&m_mutex);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool matched = false;

    if (requestId != 0) {
        for (auto &event : m_events) {
            if (event.pending && event.requestId == requestId) {
                event.pending = false;
                event.blocked = !allowed;
                event.allowed = allowed;
                event.timestamp = now;
                matched = true;
            }
        }
    }

    if (!matched) {
        for (auto &event : m_events) {
            if (event.pending && event.tabId == tab && event.requesterOrigin == requester && event.key == key) {
                event.pending = false;
                event.blocked = !allowed;
                event.allowed = allowed;
                event.timestamp = now;
                break;
            }
        }
    }
}

void ActivityAggregator::abandonPending(const QString &tab) {
    QMutexLocker locker(&m_mutex);
    for (auto &event : m_events) {
        if (event.tabId == tab) {
            event.pending = false;
        }
    }
}

void ActivityAggregator::recordPopup(const QString &tab, const QString &top, bool blocked) {
    QMutexLocker locker(&m_mutex);
    ActivityEvent event;
    event.scope = ActivityScope::Tab;
    event.kind = ActivityKind::PopupAttempt;
    event.key = QStringLiteral("popups");
    event.tabId = tab;
    event.topOrigin = top;
    event.requesterOrigin = top;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.pending = false;
    event.blocked = blocked;

    m_events.append(event);
    if (m_events.size() > m_maxEvents) {
        m_events.removeFirst();
    }
}

void ActivityAggregator::expireTab(const QString &tab) {
    QMutexLocker locker(&m_mutex);
    m_events.removeIf([&](const ActivityEvent &e) { return e.tabId == tab; });
}

ActivityAggregator::Summary ActivityAggregator::summarize(const QString &key, const QString &tab,
                                                        const QString &topOrigin, qint64 maxAgeMs) const {
    QMutexLocker locker(&m_mutex);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    Summary summary;

    for (auto it = m_events.crbegin(); it != m_events.crend() && summary.events.size() < 8; ++it) {
        if (it->key != key || it->tabId != tab || it->topOrigin != topOrigin
            || (!it->pending && (now - it->timestamp > maxAgeMs))) {
            continue;
        }
        summary.events.append(it->toMap());
        summary.pending |= it->pending;
        summary.recent = true;
        summary.blocked |= it->blocked;
        if (summary.requester.isEmpty()) {
            summary.requester = it->requesterOrigin;
        }
    }

    return summary;
}

void ActivityAggregator::pruneExpired(qint64 maxAgeMs) {
    QMutexLocker locker(&m_mutex);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_events.removeIf([now, maxAgeMs](const ActivityEvent &e) {
        return !e.pending && (now - e.timestamp > maxAgeMs);
    });
}

void ActivityAggregator::clear() {
    QMutexLocker locker(&m_mutex);
    m_events.clear();
}

qsizetype ActivityAggregator::count() const {
    QMutexLocker locker(&m_mutex);
    return m_events.size();
}

bool ActivityAggregator::isEmpty() const {
    QMutexLocker locker(&m_mutex);
    return m_events.isEmpty();
}
