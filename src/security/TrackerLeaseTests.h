#pragma once
#include "TrackerExceptionLease.h"
#include "../controlbar/RequestObserver.h"
#include <thread>
#include <atomic>

inline bool runTrackerLeaseTests() {
    TrackerExceptionLease lease, other;
    const QUrl target("https://example.test/page?exact=1");
    if (!lease.start(target) || lease.allows(target) || other.allows(target)
        || lease.canStart(QUrl("https://example.test/page?exact=2"))) return false;
    std::atomic_int consumed{0};
    auto attempt = [&] { if (lease.consume(target)) ++consumed; };
    std::thread a(attempt), b(attempt); a.join(); b.join();
    if (consumed != 1 || lease.canStart(target) || !lease.allows(target)
        || lease.allows(QUrl("https://other.test/")) || lease.allows(QUrl("http://example.test/"))
        || lease.remainingMs() <= 0 || lease.remainingMs() > 60000) return false;
    lease.revoke();
    if (lease.allows(target) || lease.remainingMs() != 0
        || lease.start(QUrl("http://example.test/")) || lease.start(QUrl("https://user:password@example.test/"))) return false;
    if (!lease.start(QUrl("https://example.test:443")) || !lease.canStart(QUrl("https://example.test/#fragment"))) return false;
    if (RequestObserver::resourceLabel(QWebEngineUrlRequestInfo::ResourceTypeXhr).isEmpty()
        || RequestObserver::resourceLabel(QWebEngineUrlRequestInfo::ResourceTypeUnknown).isEmpty()) return false;
    qInfo() << "TRACKER_LEASE_TEST PASS: HTTPS only, exact document/query, atomic single navigation, origin isolation, independent profile, deadline, revoke, credentials denied, resource labels";
    return true;
}
