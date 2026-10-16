#pragma once
#include <QtWebEngineCore/QWebEngineUrlRequestInterceptor>
#include <QtWebEngineCore/QWebEngineUrlRequestInfo>
#include <QMutex>
#include <QDateTime>
#include <QQueue>
#include <QVariantList>

// A bounded metadata sample, profile scope only. Never reads headers or bodies.
class RequestObserver final : public QWebEngineUrlRequestInterceptor {
public:
    explicit RequestObserver(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {}
    struct Event { QString origin; QString firstParty; int type; qint64 time; };
    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        const auto url = info.requestUrl();
        if (url.scheme() != "http" && url.scheme() != "https") return;
        auto sanitize = [](QUrl value) {
            return value.adjusted(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::RemoveUserInfo).toString().left(300);
        };
        Event event{sanitize(url), sanitize(info.firstPartyUrl()), int(info.resourceType()), QDateTime::currentMSecsSinceEpoch()};
        QMutexLocker lock(&mutex);
        if (events.size() == 256) events.dequeue();
        events.enqueue(std::move(event));
    }
    QVariantList sample() const {
        const auto cutoff = QDateTime::currentMSecsSinceEpoch() - 60000;
        QMutexLocker lock(&mutex);
        QVariantList result;
        for (const auto &event : events) if (event.time >= cutoff)
            result.append(QVariantMap{{"origin", event.origin}, {"firstParty", event.firstParty}, {"resourceType", event.type}, {"time", event.time}});
        return result;
    }
private:
    mutable QMutex mutex;
    QQueue<Event> events;
};
