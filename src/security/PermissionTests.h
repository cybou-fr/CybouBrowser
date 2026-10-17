#pragma once
#include "PermissionCoordinator.h"
#include "../controlbar/ControlBarController.h"
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
    if (coordinator.pendingCount() != 0 || coordinator.queuedCount() != 0 || coordinator.hasPendingFor("tab-1")) return false;
    coordinator.setActiveView(&view);
    if (coordinator.activeView() != &view) return false;
    coordinator.setActiveView(nullptr);
    if (coordinator.activeView() != nullptr) return false;

    // Multi-request queue lifecycle and atomic cancellation tests:
    PermissionCoordinator testCoord;
    QObject viewA, viewB, profileA;
    viewA.setProperty("url", QUrl("https://a.test/"));
    viewA.setProperty("tabUid", "tab-a");
    viewA.setProperty("navigationEpoch", 1);
    viewA.setProperty("profile", QVariant::fromValue<QObject *>(&profileA));

    viewB.setProperty("url", QUrl("https://b.test/"));
    viewB.setProperty("tabUid", "tab-b");
    viewB.setProperty("navigationEpoch", 10);
    viewB.setProperty("profile", QVariant::fromValue<QObject *>(&profileA));

    if (!testCoord.enqueueTestRequest(101, &viewA, &profileA, "tab-a", QUrl("https://a.test"), 1)) return false;
    if (!testCoord.enqueueTestRequest(102, &viewA, &profileA, "tab-a", QUrl("https://a.test"), 1)) return false;
    if (!testCoord.enqueueTestRequest(201, &viewB, &profileA, "tab-b", QUrl("https://b.test"), 10)) return false;
    if (!testCoord.enqueueTestRequest(202, &viewB, &profileA, "tab-b", QUrl("https://b.test"), 10)) return false;

    if (testCoord.pendingCount() != 4 || testCoord.queuedCount() != 3) return false;
    if (testCoord.pendingRequestId() != 101) return false;
    if (!testCoord.hasPendingFor("tab-a") || !testCoord.hasPendingFor("tab-b")) return false;

    testCoord.setActiveView(&viewB);
    if (testCoord.pendingRequestId() != 201 || testCoord.pendingView() != &viewB) return false;

    testCoord.documentLoaded(&viewB, 11);
    if (testCoord.hasPendingFor("tab-b")) return false;
    if (testCoord.pendingCount() != 2 || !testCoord.hasPendingFor("tab-a")) return false;

    testCoord.forgetView(&viewA);
    if (testCoord.hasPending() || testCoord.pendingCount() != 0 || testCoord.hasPendingFor("tab-a")) return false;

    testCoord.enqueueTestRequest(301, &viewA, &profileA, "tab-a", QUrl("https://a.test"), 1);
    testCoord.enqueueTestRequest(302, &viewA, &profileA, "tab-a", QUrl("https://a.test"), 1);
    testCoord.enqueueTestRequest(303, &viewB, &profileA, "tab-b", QUrl("https://b.test"), 1);
    if (testCoord.pendingCount() != 3) return false;
    testCoord.revokeAll();
    if (testCoord.hasPending() || testCoord.pendingCount() != 0 || testCoord.queuedCount() != 0) return false;
    PermissionDeadline expired(0), running(60000);
    if (!expired.expired() || running.expired()) return false;
    // Clock rollback must not extend the monotonic ceiling; forward jumps fail closed.
    running.wallExpiresAt += 3600000;
    if (running.remainingTime() > 60000) return false;
    running.wallExpiresAt = QDateTime::currentMSecsSinceEpoch() - 1;
    if (!running.expired()) return false;
    if (!ControlBarController::origin(QUrl("https:///path-without-host")).isEmpty()) return false;
    QQuickWebEngineProfile webProfile;
    ControlBarController controller;
    controller.setProfile(&webProfile);
    controller.setTabId("tab-1");
    controller.setUrl(QUrl("https://top.example/article"));
    if (controller.capability("camera").value("explicitDeny").toBool()) return false;
    if (!controller.setPermissionPolicy("camera", 2)
        || !controller.capability("camera").value("explicitDeny").toBool()) return false;
    if (!controller.shouldDenyOrigin("camera", QUrl("https://iframe.example/widget"), QUrl("https://top.example/article"), "tab-1")
        || controller.shouldDenyOrigin("camera", QUrl("https://iframe.example/widget"), QUrl("https://top.example/article"), "tab-2")
        || controller.shouldDenyOrigin("mic", QUrl("https://iframe.example/widget"), QUrl("https://top.example/article"), "tab-1")) return false;
    controller.inheritContext("tab-1", "tab-3", QUrl("https://top.example/article"), QUrl("https://top.example/article"));
    if (!controller.shouldDenyOrigin("camera", QUrl("https://iframe.example/widget"), QUrl("https://top.example/article"), "tab-3")) return false;
    qInfo() << "PERMISSION_CONTEXT_TEST PASS: view/profile/tab/top-origin/epoch, iframe requester, top-level deny propagation, destroyed context, invalid request and dual-clock deadline";
    return true;
}
