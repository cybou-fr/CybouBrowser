#pragma once
#include "HttpNavigationLease.h"
#include <QDebug>
#include <thread>
#include <atomic>

inline bool runHttpLeaseTests() {
    const QUrl target("http://example.test/document?attempt=1");
    HttpNavigationLease first, other;
    if (!first.start(target) || !first.canStart(target)
        || first.canStart(QUrl("http://example.test/document?attempt=2"))
        || first.allows(QUrl("http://example.test/asset"), target, false)
        || other.allows(target, target, true)) return false;
    std::atomic_int accepted{0};
    auto attempt = [&] { if (first.allows(target, target, true)) ++accepted; };
    std::thread a(attempt), b(attempt); a.join(); b.join();
    if (accepted != 1 || first.canStart(target) || first.allows(target, target, true)
        || !first.allows(QUrl("http://example.test/asset"), target, false)
        || first.allows(QUrl("http://other.test/asset"), target, false)
        || first.allows(QUrl("http://example.test/asset"), QUrl("http://other.test/"), false)
        || first.allows(QUrl("ws://example.test/socket"), target, false)) return false;
    first.revoke();
    if (first.allows(QUrl("http://example.test/asset"), target, false)) return false;
    HttpNavigationLease root;
    if (!root.start(QUrl("http://example.test:80")) || !root.canStart(QUrl("http://example.test/#fragment"))
        || root.start(QUrl("http://user:secret@example.test/")) || root.start(QUrl("https://example.test/"))) return false;
    qInfo() << "HTTP_LEASE_TEST PASS: exact URL/query, one main request under concurrency, same-origin resources only, profile separation, credentials/ws denied, canonical root, revoke";
    return true;
}
