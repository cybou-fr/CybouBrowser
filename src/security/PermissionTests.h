#pragma once
#include "PermissionCoordinator.h"
#include <QDebug>
#include <memory>

inline bool runPermissionContextTests() {
    QObject view, otherView, profile, otherProfile;
    view.setProperty("url", QUrl("https://top.example/page"));
    view.setProperty("tabUid", "tab-1");
    view.setProperty("navigationEpoch", 7);
    view.setProperty("profile", QVariant::fromValue<QObject *>(&profile));
    SiteContext context{&view, &profile, "tab-1", QUrl("https://top.example"), QUrl("https://frame.example"), 7};
    if (!context.matches(&view, &profile) || context.matches(&otherView, &profile)
        || context.matches(&view, &otherProfile)) return false;
    view.setProperty("navigationEpoch", 8);
    if (context.matches(&view, &profile)) return false;
    view.setProperty("navigationEpoch", 7);
    view.setProperty("tabUid", "tab-2");
    if (context.matches(&view, &profile)) return false;
    view.setProperty("tabUid", "tab-1");
    view.setProperty("url", QUrl("https://attacker.example/page"));
    if (context.matches(&view, &profile)) return false;
    view.setProperty("url", QUrl("https://top.example:443/another"));
    if (!context.matches(&view, &profile) || context.requestingOrigin == context.topOrigin) return false;
    auto deletedProfile = std::make_unique<QObject>();
    context.profile = deletedProfile.get();
    deletedProfile.reset();
    if (context.matches(&view, nullptr)) return false;
    PermissionCoordinator coordinator;
    if (coordinator.submit(QWebEnginePermission(), &view, &profile, true, false)
        || coordinator.hasPending() || coordinator.resolve(true, &view, &profile, 1)) return false;
    PermissionDeadline expired(0), running(60000);
    if (!expired.expired() || running.expired()) return false;
    // Clock rollback must not extend the monotonic ceiling; forward jumps fail closed.
    running.wallExpiresAt += 3600000;
    if (running.remainingTime() > 60000) return false;
    running.wallExpiresAt = QDateTime::currentMSecsSinceEpoch() - 1;
    if (!running.expired()) return false;
    qInfo() << "PERMISSION_CONTEXT_TEST PASS: view/profile/tab/top-origin/epoch, iframe requester, destroyed context, invalid request and dual-clock deadline";
    return true;
}
