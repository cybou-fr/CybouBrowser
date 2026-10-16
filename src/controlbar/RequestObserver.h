#pragma once
#include <QtWebEngineCore/QWebEngineUrlRequestInterceptor>
#include <QtWebEngineCore/QWebEngineUrlRequestInfo>
#include <QMutex>
#include <QDateTime>
#include <QQueue>
#include <QVariantList>
#include <atomic>
#include "../security/NetworkPolicy.h"
#include "../security/TrackerBlocker.h"

// A bounded metadata sample, profile scope only. Never reads headers or bodies.
class RequestObserver final : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    explicit RequestObserver(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {}
    void setHttpsOnly(bool enabled) { strictHttps.store(enabled); }
    void setBlockTrackers(bool enabled) { blockTrackers.store(enabled); }
    int blockedCount() const { return blockedTrackersCount.load(); }
    struct Event { QString origin; QString firstParty; int type; qint64 time; bool blocked; QString reason; };
    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        const auto url = info.requestUrl();
        QString reason;
        if (NetworkPolicy::blocks(url, strictHttps.load())) reason = "insecure-transport";
        else if (NetworkPolicy::hasWebCredentials(url)) reason = "embedded-credentials";
        else if (blockTrackers.load() && info.resourceType() != QWebEngineUrlRequestInfo::ResourceTypeMainFrame
                 && TrackerBlocker::isTracker(url, info.firstPartyUrl())) {
            reason = "tracker-rule";
            blockedTrackersCount.fetch_add(1, std::memory_order_relaxed);
        }
        const bool blocked = !reason.isEmpty();
        if (blocked) info.block(true);
        if (url.scheme() != "http" && url.scheme() != "https" && url.scheme() != "ws") return;
        auto sanitize = [](QUrl value) {
            return value.adjusted(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::RemoveUserInfo).toString().left(300);
        };
        Event event{sanitize(url), sanitize(info.firstPartyUrl()), int(info.resourceType()), QDateTime::currentMSecsSinceEpoch(), blocked, reason};
        QMutexLocker lock(&mutex);
        if (events.size() == 256) events.dequeue();
        events.enqueue(std::move(event));
        lock.unlock();
        if (!notificationPending.exchange(true)) emit activityAvailable();
    }
    QVariantList sample() const {
        notificationPending.store(false);
        const auto cutoff = QDateTime::currentMSecsSinceEpoch() - 60000;
        QMutexLocker lock(&mutex);
        QVariantList result;
        for (const auto &event : events) if (event.time >= cutoff)
            result.append(QVariantMap{{"origin", event.origin}, {"firstParty", event.firstParty}, {"resourceType", event.type}, {"time", event.time}, {"blocked", event.blocked}, {"reason", event.reason}, {"scope", "profile"}});
        return result;
    }
signals:
    void activityAvailable();
private:
    mutable std::atomic_bool notificationPending{false};
    std::atomic_bool strictHttps{false};
    std::atomic_bool blockTrackers{true};
    std::atomic_int blockedTrackersCount{0};
    mutable QMutex mutex;
    QQueue<Event> events;
};
