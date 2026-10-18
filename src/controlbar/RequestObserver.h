#pragma once
#include <QtWebEngineCore/QWebEngineUrlRequestInterceptor>
#include <QtWebEngineCore/QWebEngineUrlRequestInfo>
#include <QMutex>
#include <QHostAddress>
#include <QHostInfo>
#include <functional>
#include <QDateTime>
#include <QQueue>
#include <QVariantList>
#include <atomic>
#include "../security/NetworkPolicy.h"
#include "../security/RuleEngine.h"
#include "../security/TrackerBlocker.h"
#include "../security/HttpNavigationLease.h"
#include "../security/TrackerExceptionRegistry.h"
#include "../security/NavigationGuard.h"

// A bounded metadata sample, profile scope only. Never reads headers or bodies.
class RequestObserver final : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    explicit RequestObserver(QObject *parent) : QWebEngineUrlRequestInterceptor(parent) {}
    // Best-effort name resolution for WebSocket targets from public pages (engine PNA does not
    // cover WebSocket). Not bound to the later connection, so it is no DNS-rebinding defence.
    using HostResolver = std::function<QList<QHostAddress>(const QString &)>;
    void setHostResolver(HostResolver resolver) { QMutexLocker locker(&resolverMutex); hostResolver = std::move(resolver); }
    void setHttpsOnly(bool enabled) { strictHttps.store(enabled); }
    void setBlockTrackers(bool enabled) { blockTrackers.store(enabled); }
    int blockedCount() const { return blockedTrackersCount.load() + blockedAdsCount.load(); }
    int blockedTrackers() const { return blockedTrackersCount.load(); }
    int blockedAds() const { return blockedAdsCount.load(); }

    bool allowHttpNavigation(const QUrl &url) { return httpLease.start(url); }
    bool canStartHttpNavigation(const QUrl &url) const { return httpLease.canStart(url); }
    void revokeHttpNavigation() { httpLease.revoke(); }

    bool allowTrackersForOrigin(const QUrl &url, QObject *owner, bool testHttp = false) {
        return trackerRegistry.grant(url, owner, TrackerExceptionRegistry::ExceptionTrackers, testHttp);
    }
    bool allowAdsForOrigin(const QUrl &url, QObject *owner, bool testHttp = false) {
        return trackerRegistry.grant(url, owner, TrackerExceptionRegistry::ExceptionAds, testHttp);
    }
    bool isTrackerAllowed(const QUrl &url) const { return trackerRegistry.allowsTrackers(url); }
    bool isAdAllowed(const QUrl &url) const { return trackerRegistry.allowsAds(url); }
    int allowedExceptionTypes(const QUrl &url) const { return trackerRegistry.allowedTypesFor(url); }

    bool revokeTrackersForOrigin(const QUrl &url) {
        return trackerRegistry.revoke(url, TrackerExceptionRegistry::ExceptionTrackers);
    }
    bool revokeAdsForOrigin(const QUrl &url) {
        return trackerRegistry.revoke(url, TrackerExceptionRegistry::ExceptionAds);
    }
    void retainTrackersForOwner(const QObject *owner, const QUrl &currentUrl) {
        trackerRegistry.retainForOwner(owner, currentUrl);
    }
    void releaseTrackerOwner(const QObject *owner) {
        trackerRegistry.releaseOwner(owner);
    }

    struct Event { QString origin; QString firstParty; int type; qint64 time; bool blocked; QString reason; QString decision; };

    bool webSocketResolvesPrivate(const QUrl &url, const QUrl &firstParty) {
        const QString host = url.host();
        QHostAddress literal;
        if (host.isEmpty() || firstParty.isEmpty() || !firstParty.isValid() || NavigationGuard::isPrivateNetwork(firstParty) || literal.setAddress(host)) return false;
        HostResolver resolver;
        { QMutexLocker locker(&resolverMutex); resolver = hostResolver; }
        const auto addresses = resolver ? resolver(host) : QHostInfo::fromName(host).addresses();
        for (const auto &address : addresses)
            if (NavigationGuard::isPrivateNetworkAddress(address)) return true;
        return false;
    }

    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        const auto url = info.requestUrl();
        const auto firstParty = info.firstPartyUrl();
        const bool isMainFrame = (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame);
        QString reason, decision;

        if (NetworkPolicy::hasWebCredentials(url)) {
            reason = "embedded-credentials";
        } else if (NetworkPolicy::blocks(url, strictHttps.load())
                 && !httpLease.allows(url, firstParty, isMainFrame)) {
            reason = "insecure-transport";
        } else if (NavigationGuard::blocksPrivateNetworkAccess(url, firstParty, isMainFrame)) {
            reason = "private-network-access";
        } else if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeWebSocket && webSocketResolvesPrivate(url, firstParty)) {
            reason = "private-network-access";
        } else if (blockTrackers.load() && !isMainFrame) {
            const auto match = RuleEngine::classify(url, firstParty);
            if (match.matched) {
                if (match.category == RuleEngine::RuleCategory::Ad) {
                    if (trackerRegistry.allowsAds(firstParty)) {
                        decision = "ad-exception";
                    } else {
                        reason = "ad-rule";
                        blockedAdsCount.fetch_add(1, std::memory_order_relaxed);
                    }
                } else {
                    if (trackerRegistry.allowsTrackers(firstParty)) {
                        decision = "tracker-exception";
                    } else {
                        reason = "tracker-rule";
                        blockedTrackersCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
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
            const QString category = (event.reason == "insecure-transport" || event.reason == "private-network-access") ? "transport"
                : (event.reason == "ad-rule" || event.decision == "ad-exception") ? "ads"
                : (event.reason == "tracker-rule" || event.decision == "tracker-exception") ? "trackers" : "other";

            const QString explanation = event.reason == "insecure-transport" ? tr("Connexion HTTP ou WebSocket non chiffrée")
                : event.reason == "private-network-access" ? tr("Requête d’un site public vers le réseau local bloquée (Navigation Guard)")
                : event.reason == "ad-rule" ? tr("Domaine publicitaire tiers bloqué par le filtre")
                : event.reason == "tracker-rule" ? tr("Domaine tiers présent dans la liste de traqueurs")
                : event.reason == "embedded-credentials" ? tr("Identifiants intégrés à l’adresse")
                : event.reason == "temporary-navigation" ? tr("Nouvelle navigation : l’exception a pris fin")
                : event.decision == "ad-exception" ? tr("Publicité autorisée par l’exception temporaire")
                : event.decision == "tracker-exception" ? tr("Traqueur autorisé par l’exception temporaire")
                : tr("Requête passée par le filtre réseau");

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
    std::atomic_int blockedAdsCount{0};
    QMutex resolverMutex;
    HostResolver hostResolver;
    mutable QMutex mutex;
    QQueue<Event> events;
    HttpNavigationLease httpLease;
    TrackerExceptionRegistry trackerRegistry;
};
