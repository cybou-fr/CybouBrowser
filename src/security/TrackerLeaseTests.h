#pragma once
#include "TrackerExceptionRegistry.h"
#include "RuleEngine.h"
#include "../controlbar/RequestObserver.h"
#include <QObject>
#include <memory>

inline bool runTrackerLeaseTests() {
    // 1. RuleEngine Classification & False-Positive Immunity Tests
    {
        // Ads classification
        const auto ad1 = RuleEngine::classify(QUrl("https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js"), QUrl("https://example.com"));
        if (!ad1.matched || ad1.category != RuleEngine::RuleCategory::Ad) return false;

        const auto ad2 = RuleEngine::classify(QUrl("https://adservice.google.com/adsid/integrator.js"), QUrl("https://news.com"));
        if (!ad2.matched || ad2.category != RuleEngine::RuleCategory::Ad) return false;

        const auto ad3 = RuleEngine::classify(QUrl("https://secure.adnxs.com/seg?add=1"), QUrl("https://shop.com"));
        if (!ad3.matched || ad3.category != RuleEngine::RuleCategory::Ad) return false;

        // Trackers classification
        const auto tr1 = RuleEngine::classify(QUrl("https://www.google-analytics.com/analytics.js"), QUrl("https://example.com"));
        if (!tr1.matched || tr1.category != RuleEngine::RuleCategory::Tracker) return false;

        const auto tr2 = RuleEngine::classify(QUrl("https://connect.facebook.net/en_US/fbevents.js"), QUrl("https://shop.com"));
        if (!tr2.matched || tr2.category != RuleEngine::RuleCategory::Tracker) return false;

        const auto tr3 = RuleEngine::classify(QUrl("https://cdn.segment.com/analytics.js/v1/key/analytics.min.js"), QUrl("https://app.com"));
        if (!tr3.matched || tr3.category != RuleEngine::RuleCategory::Tracker) return false;

        // First-party exemptions
        const auto fp1 = RuleEngine::classify(QUrl("https://adservice.google.com/test"), QUrl("https://adservice.google.com/"));
        if (fp1.matched) return false;

        const auto fp2 = RuleEngine::classify(QUrl("https://analytics.google.com/test"), QUrl("https://google.com"));
        if (fp2.matched) return false;

        // False positive immunity: critical CDNs, payment gateways, authentication, CAPTCHAs
        const auto cdn = RuleEngine::classify(QUrl("https://cdnjs.cloudflare.com/ajax/libs/react/18.0.0/react.production.min.js"), QUrl("https://site.com"));
        if (cdn.matched) return false;

        const auto pay1 = RuleEngine::classify(QUrl("https://js.stripe.com/v3/"), QUrl("https://store.com"));
        if (pay1.matched) return false;

        const auto pay2 = RuleEngine::classify(QUrl("https://checkout.stripe.com/pay"), QUrl("https://store.com"));
        if (pay2.matched) return false;

        const auto auth = RuleEngine::classify(QUrl("https://accounts.google.com/o/oauth2/auth"), QUrl("https://client.com"));
        if (auth.matched) return false;

        const auto captcha = RuleEngine::classify(QUrl("https://recaptcha.net/recaptcha/api.js"), QUrl("https://form.com"));
        if (captcha.matched) return false;
    }

    // 2. Granular Independent Exceptions Tests
    TrackerExceptionRegistry registry;
    auto ownerA = std::make_unique<QObject>();
    auto ownerB = std::make_unique<QObject>();

    const QUrl targetA("https://example.test/page?exact=1");
    const QUrl targetB("https://another.test/foo");
    const QUrl httpUrl("http://example.test/");
    const QUrl credUrl("https://user:password@example.test/");

    // Grant Trackers only to targetA
    if (!registry.grant(targetA, ownerA.get(), TrackerExceptionRegistry::ExceptionTrackers)) return false;
    if (!registry.allowsTrackers(targetA)) return false;
    if (registry.allowsAds(targetA)) return false; // Ads must remain blocked!

    // Cross origin checks
    if (registry.allowsTrackers(QUrl("https://other.test/"))) return false;
    // Same origin different path/query allowed
    if (!registry.allowsTrackers(QUrl("https://example.test/other-path?q=2"))) return false;

    // Grant Ads only to targetB
    if (!registry.grant(targetB, ownerB.get(), TrackerExceptionRegistry::ExceptionAds)) return false;
    if (!registry.allowsAds(targetB)) return false;
    if (registry.allowsTrackers(targetB)) return false; // Trackers must remain blocked!
    if (registry.size() != 2) return false;

    // Credentials / HTTP rejection
    if (registry.grant(httpUrl, ownerA.get()) || registry.grant(credUrl, ownerA.get())) return false;

    // Add Ads exception to targetA as well (both active independently)
    if (!registry.grant(targetA, ownerA.get(), TrackerExceptionRegistry::ExceptionAds)) return false;
    if (!registry.allowsTrackers(targetA) || !registry.allowsAds(targetA)) return false;

    // Partially revoke Ads on targetA; Trackers must remain active!
    if (!registry.revoke(targetA, TrackerExceptionRegistry::ExceptionAds)) return false;
    if (!registry.allowsTrackers(targetA)) return false;
    if (registry.allowsAds(targetA)) return false;

    // Retain for owner when staying on origin
    registry.retainForOwner(ownerA.get(), QUrl("https://example.test/checkout"));
    if (!registry.allowsTrackers(targetA)) return false;

    // Retain for owner when navigating away from origin
    registry.retainForOwner(ownerA.get(), QUrl("https://third.test/page"));
    if (registry.allowsTrackers(targetA)) return false;
    // targetB still active
    if (!registry.allowsAds(targetB)) return false;

    // Manual revoke targetB completely
    registry.revoke(targetB);
    if (registry.allowsAds(targetB)) return false;

    // Owner destruction clears entry
    auto ownerC = std::make_unique<QObject>();
    if (!registry.grant(targetA, ownerC.get(), TrackerExceptionRegistry::ExceptionAll)) return false;
    if (!registry.allowsTrackers(targetA) || !registry.allowsAds(targetA)) return false;
    ownerC.reset();
    if (registry.allowsTrackers(targetA) || registry.allowsAds(targetA)) return false;

    if (RequestObserver::resourceLabel(QWebEngineUrlRequestInfo::ResourceTypeXhr).isEmpty()
        || RequestObserver::resourceLabel(QWebEngineUrlRequestInfo::ResourceTypeUnknown).isEmpty()) return false;

    qInfo() << "TRACKER_LEASE_TEST PASS: RuleEngine Ads/Trackers classification, false-positive immunity, independent exceptions, origin scope, owner retention and lifecycle";
    return true;
}
