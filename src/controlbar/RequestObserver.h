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
#include "../security/HttpNavigationLease.h"
#include "../security/TrackerExceptionLease.h"

// A bounded metadata sample, profile scope only. Never reads headers or bodies.
class RequestObserver final : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    explicit RequestObserver(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {}
    void setHttpsOnly(bool enabled) { strictHttps.store(enabled); }
    void setBlockTrackers(bool enabled) { blockTrackers.store(enabled); }
    int blockedCount() const { return blockedTrackersCount.load(); }
    bool allowHttpNavigation(const QUrl &url) { return httpLease.start(url); }
    bool canStartHttpNavigation(const QUrl &url) const { return httpLease.canStart(url); }
    void revokeHttpNavigation() { httpLease.revoke(); }
    bool allowTrackersForDocument(const QUrl &url, bool testHttp = false) { trackerProfile = trackerLease.start(url, testHttp); return trackerProfile; }
    bool canStartTrackerDocument(const QUrl &url) const { return trackerLease.canStart(url); }
    qint64 trackerRemainingMs() const { return trackerLease.remainingMs(); }
    void revokeTrackers() { trackerLease.revoke(); }
    struct Event { QString origin; QString firstParty; int type; qint64 time; bool blocked; QString reason; QString decision; };
    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        const auto url = info.requestUrl();
        QString reason, decision;
        if (NetworkPolicy::hasWebCredentials(url)) reason = "embedded-credentials";
        else if (NetworkPolicy::blocks(url, strictHttps.load())
                 && !httpLease.allows(url, info.firstPartyUrl(), info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame)) reason = "insecure-transport";
        else if (trackerProfile && info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame && !trackerLease.consume(url)) reason = "temporary-navigation";
        else if (blockTrackers.load() && info.resourceType() != QWebEngineUrlRequestInfo::ResourceTypeMainFrame
                 && TrackerBlocker::isTracker(url, info.firstPartyUrl())) {
            if (trackerLease.allows(info.firstPartyUrl())) decision = "tracker-exception";
            else { reason = "tracker-rule"; blockedTrackersCount.fetch_add(1, std::memory_order_relaxed); }
        }
        const bool blocked = !reason.isEmpty();
        if (blocked) info.block(true);
        if (url.scheme() != "http" && url.scheme() != "https" && url.scheme() != "ws" && url.scheme() != "wss") return;
        auto sanitize = [](QUrl value) {
            return value.adjusted(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::RemoveUserInfo).toString().left(300);
        };
        Event event{sanitize(url), sanitize(info.firstPartyUrl()), int(info.resourceType()), QDateTime::currentMSecsSinceEpoch(), blocked, reason, decision};
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
        for (const auto &event : events) if (event.time >= cutoff) {
            const QString category = event.reason == "insecure-transport" ? "transport" : event.reason == "tracker-rule" || event.decision == "tracker-exception" ? "trackers" : "other";
            const QString explanation = event.reason == "insecure-transport" ? tr("Connexion HTTP ou WebSocket non chiffrée")
                : event.reason == "tracker-rule" ? tr("Domaine tiers présent dans la liste de traqueurs")
                : event.reason == "embedded-credentials" ? tr("Identifiants intégrés à l’adresse")
                : event.reason == "temporary-navigation" ? tr("Nouvelle navigation : l’exception a pris fin")
                : event.decision == "tracker-exception" ? tr("Traqueur autorisé par l’exception temporaire") : tr("Requête passée par le filtre réseau");
            result.append(QVariantMap{{"origin", event.origin}, {"firstParty", event.firstParty}, {"resourceType", event.type}, {"resourceLabel", resourceLabel(event.type)},
                {"time", event.time}, {"blocked", event.blocked}, {"reason", event.reason}, {"decision", event.decision}, {"category", category}, {"explanation", explanation}, {"scope", "profile"}});
        }
        return result;
    }
    static QString resourceLabel(int type) {
        using Info = QWebEngineUrlRequestInfo;
        switch (type) {
        case Info::ResourceTypeMainFrame: return tr("Page");
        case Info::ResourceTypeSubFrame: return tr("Cadre intégré");
        case Info::ResourceTypeStylesheet: return tr("Feuille de style");
        case Info::ResourceTypeScript: return tr("Script");
        case Info::ResourceTypeImage: return tr("Image");
        case Info::ResourceTypeFontResource: return tr("Police");
        case Info::ResourceTypeMedia: return tr("Média");
        case Info::ResourceTypeWorker: case Info::ResourceTypeSharedWorker: case Info::ResourceTypeServiceWorker: return tr("Worker");
        case Info::ResourceTypeFavicon: return tr("Icône du site");
        case Info::ResourceTypeXhr: return tr("Requête de données");
        case Info::ResourceTypePing: return tr("Signal de suivi");
        case Info::ResourceTypeCspReport: return tr("Rapport de sécurité");
        case Info::ResourceTypePrefetch: return tr("Préchargement");
        case Info::ResourceTypeWebSocket: return tr("WebSocket");
        default: return tr("Autre ressource");
        }
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
    HttpNavigationLease httpLease;
    std::atomic_bool trackerProfile{false};
    TrackerExceptionLease trackerLease;
};
