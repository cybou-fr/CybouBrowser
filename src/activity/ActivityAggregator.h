#pragma once
#include "ActivityEvent.h"
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantList>

class ActivityAggregator {
public:
    struct Summary {
        bool pending = false;
        bool recent = false;
        bool blocked = false;
        QString requester;
        QVariantList events;
    };

    explicit ActivityAggregator(qsizetype maxEvents = 256);

    void recordPermission(const QString &key, const QString &tab, const QString &top,
                          const QString &requester, bool pending, bool blocked, quint64 requestId = 0);
    void finishPermission(const QString &key, const QString &tab, const QString &requester,
                          bool allowed, quint64 requestId = 0);
    void abandonPending(const QString &tab);
    void recordPopup(const QString &tab, const QString &top, bool blocked);
    void expireTab(const QString &tab);

    Summary summarize(const QString &key, const QString &tab, const QString &topOrigin, qint64 maxAgeMs = 60000) const;

    void pruneExpired(qint64 maxAgeMs = 60000);
    void clear();

    qsizetype count() const;
    bool isEmpty() const;

private:
    qsizetype m_maxEvents;
    mutable QMutex m_mutex;
    QList<ActivityEvent> m_events;
};
