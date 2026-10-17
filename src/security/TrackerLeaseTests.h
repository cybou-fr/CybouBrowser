#pragma once
#include "TrackerExceptionRegistry.h"
#include "../controlbar/RequestObserver.h"
#include <QObject>
#include <memory>

inline bool runTrackerLeaseTests() {
    TrackerExceptionRegistry registry;
    auto ownerA = std::make_unique<QObject>();
    auto ownerB = std::make_unique<QObject>();

    const QUrl targetA("https://example.test/page?exact=1");
    const QUrl targetB("https://another.test/foo");
    const QUrl httpUrl("http://example.test/");
    const QUrl credUrl("https://user:password@example.test/");

    if (!registry.grant(targetA, ownerA.get()) || !registry.allows(targetA)) return false;
    // Cross origin checks
    if (registry.allows(QUrl("https://other.test/"))) return false;
    // Same origin different path/query allowed
    if (!registry.allows(QUrl("https://example.test/other-path?q=2"))) return false;

    // Grant for second origin concurrently
    if (!registry.grant(targetB, ownerB.get()) || !registry.allows(targetB)) return false;
    if (registry.size() != 2) return false;

    // Credentials / HTTP rejection
    if (registry.grant(httpUrl, ownerA.get()) || registry.grant(credUrl, ownerA.get())) return false;

    // Retain for owner when staying on origin
    registry.retainForOwner(ownerA.get(), QUrl("https://example.test/checkout"));
    if (!registry.allows(targetA)) return false;

    // Retain for owner when navigating away from origin
    registry.retainForOwner(ownerA.get(), QUrl("https://third.test/page"));
    if (registry.allows(targetA)) return false;
    // targetB still active
    if (!registry.allows(targetB)) return false;

    // Manual revoke
    registry.revoke(targetB);
    if (registry.allows(targetB)) return false;

    // Owner destruction clears entry
    auto ownerC = std::make_unique<QObject>();
    if (!registry.grant(targetA, ownerC.get())) return false;
    if (!registry.allows(targetA)) return false;
    ownerC.reset();
    if (registry.allows(targetA)) return false;

    if (RequestObserver::resourceLabel(QWebEngineUrlRequestInfo::ResourceTypeXhr).isEmpty()
        || RequestObserver::resourceLabel(QWebEngineUrlRequestInfo::ResourceTypeUnknown).isEmpty()) return false;

    qInfo() << "TRACKER_LEASE_TEST PASS: HTTPS only, origin scope, multi-origin support, owner lifetime retention, destruction cleanup, credentials denied, resource labels";
    return true;
}
