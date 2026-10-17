#pragma once
#include "PermissionCoordinator.h"
#include "EffectivePermissionPolicy.h"
#include "../controlbar/ControlBarController.h"
#include <QDebug>
#include <memory>

inline bool runCrossOriginIframeMatrixTests() {
    // 1. Direct evaluation with EffectivePermissionPolicy:
    const QUrl topA("https://top-a.example/page");
    const QUrl sameTopA("https://top-a.example:443/embed");
    const QUrl frameB("https://frame-b.example/widget");
    const QUrl frameC("https://frame-c.example/widget");
    const QUrl portFrame("https://frame-b.example:8443/widget");

    // 1a. Normalization and Cross-origin detection
    auto evalSame = EffectivePermissionPolicy::evaluate("camera", topA, sameTopA, 0, 0);
    if (evalSame.isCrossOrigin || evalSame.decision != PermissionDecision::Ask) {
        qCritical() << "IFRAME_MATRIX FAIL: same origin detected as cross-origin";
        return false;
    }
    auto evalCross = EffectivePermissionPolicy::evaluate("camera", topA, frameB, 0, 0);
    if (!evalCross.isCrossOrigin || evalCross.decision != PermissionDecision::Ask) {
        qCritical() << "IFRAME_MATRIX FAIL: cross-origin not detected";
        return false;
    }
    auto evalPortCross = EffectivePermissionPolicy::evaluate("camera", frameB, portFrame, 0, 0);
    if (!evalPortCross.isCrossOrigin) {
        qCritical() << "IFRAME_MATRIX FAIL: different port not detected as cross-origin";
        return false;
    }

    // 1b. Screen sharing permanently denied
    auto evalScreen = EffectivePermissionPolicy::evaluate("screen", topA, frameB, 0, 0);
    if (evalScreen.decision != PermissionDecision::Deny) {
        qCritical() << "IFRAME_MATRIX FAIL: screen not denied";
        return false;
    }

    // 1c. Top-level deny cascades to all requesters
    auto evalTopDeny = EffectivePermissionPolicy::evaluate("camera", topA, frameB, 2, 0);
    if (evalTopDeny.decision != PermissionDecision::Deny || !evalTopDeny.deniedByTop || evalTopDeny.deniedByRequester) {
        qCritical() << "IFRAME_MATRIX FAIL: top deny cascade";
        return false;
    }

    // 1d. Requester-specific deny affects only that requester
    auto evalReqDeny = EffectivePermissionPolicy::evaluate("camera", topA, frameB, 0, 2);
    if (evalReqDeny.decision != PermissionDecision::Deny || evalReqDeny.deniedByTop || !evalReqDeny.deniedByRequester) {
        qCritical() << "IFRAME_MATRIX FAIL: requester deny";
        return false;
    }
    auto evalOtherReq = EffectivePermissionPolicy::evaluate("camera", topA, frameC, 0, 0);
    if (evalOtherReq.decision != PermissionDecision::Ask) {
        qCritical() << "IFRAME_MATRIX FAIL: frameB deny leaked to frameC";
        return false;
    }

    // 1e. Compound permission evaluation
    auto evalCompoundSafe = EffectivePermissionPolicy::evaluateCompound({"camera", "mic"}, topA, frameB,
        [](const QString &, const QUrl &) { return 0; });
    if (evalCompoundSafe.decision != PermissionDecision::Ask) {
        qCritical() << "IFRAME_MATRIX FAIL: compound safe denied";
        return false;
    }
    auto evalCompoundReqDeny = EffectivePermissionPolicy::evaluateCompound({"camera", "mic"}, topA, frameB,
        [&frameB](const QString &k, const QUrl &o) {
            return (k == "mic" && o == frameB) ? 2 : 0;
        });
    if (evalCompoundReqDeny.decision != PermissionDecision::Deny || !evalCompoundReqDeny.deniedByRequester) {
        qCritical() << "IFRAME_MATRIX FAIL: compound requester mic deny failed";
        return false;
    }
    auto evalCompoundTopDeny = EffectivePermissionPolicy::evaluateCompound({"camera", "mic"}, topA, frameB,
        [&topA](const QString &k, const QUrl &o) {
            return (k == "camera" && o == topA) ? 2 : 0;
        });
    if (evalCompoundTopDeny.decision != PermissionDecision::Deny || !evalCompoundTopDeny.deniedByTop) {
        qCritical() << "IFRAME_MATRIX FAIL: compound top camera deny failed";
        return false;
    }

    // 2. ControlBarController Integration Matrix:
    QQuickWebEngineProfile profile;
    ControlBarController controller;
    controller.setProfile(&profile);
    controller.setTabId("tab-main");
    controller.setUrl(topA);

    if (controller.shouldDenyOrigin("camera", frameB, topA, "tab-main")) return false;
    if (controller.shouldDenyOrigin("location", frameB, topA, "tab-main")) return false;

    if (!controller.setPermissionPolicy("location", 2, frameB)) return false;

    if (!controller.shouldDenyOrigin("location", frameB, topA, "tab-main")) {
        qCritical() << "IFRAME_MATRIX FAIL: targetUrl frameB location not denied";
        return false;
    }
    if (controller.shouldDenyOrigin("location", frameC, topA, "tab-main")) {
        qCritical() << "IFRAME_MATRIX FAIL: frameB location deny leaked to frameC";
        return false;
    }
    if (controller.shouldDenyOrigin("location", topA, topA, "tab-main")) {
        qCritical() << "IFRAME_MATRIX FAIL: frameB location deny leaked to topA";
        return false;
    }
    if (controller.shouldDenyOrigin("location", frameB, topA, "tab-other")) {
        qCritical() << "IFRAME_MATRIX FAIL: tab-main deny leaked to tab-other";
        return false;
    }

    if (!controller.setPermissionPolicy("camera", 2)) return false;

    if (!controller.shouldDenyOrigin("camera", topA, topA, "tab-main")
        || !controller.shouldDenyOrigin("camera", frameB, topA, "tab-main")
        || !controller.shouldDenyOrigin("camera", frameC, topA, "tab-main")) {
        qCritical() << "IFRAME_MATRIX FAIL: topA camera deny did not cascade to frames";
        return false;
    }

    // 3. Tab Expiration and Context:
    controller.expireContext("tab-main");
    if (controller.shouldDenyOrigin("location", frameB, topA, "tab-main")
        || controller.shouldDenyOrigin("camera", frameB, topA, "tab-main")) {
        qCritical() << "IFRAME_MATRIX FAIL: expireContext did not clean up tab-main policies";
        return false;
    }

    // 4. PermissionCoordinator Cross-Origin Enqueue, Priority, and Epoch cancellation:
    PermissionCoordinator coordinator;
    QObject view1, view2, webProf;
    view1.setProperty("url", topA);
    view1.setProperty("tabUid", "tab-1");
    view1.setProperty("navigationEpoch", 5);
    view1.setProperty("profile", QVariant::fromValue<QObject *>(&webProf));

    view2.setProperty("url", topA);
    view2.setProperty("tabUid", "tab-2");
    view2.setProperty("navigationEpoch", 1);
    view2.setProperty("profile", QVariant::fromValue<QObject *>(&webProf));

    coordinator.setActiveView(&view1);

    if (!coordinator.enqueueTestRequest(501, &view1, &webProf, "tab-1", frameB, 5)) return false;
    if (!coordinator.enqueueTestRequest(502, &view2, &webProf, "tab-2", frameC, 1)) return false;

    if (coordinator.pendingRequestId() != 501 || coordinator.pendingView() != &view1) return false;

    coordinator.setActiveView(&view2);
    if (coordinator.pendingRequestId() != 502 || coordinator.pendingView() != &view2) return false;

    coordinator.documentLoaded(&view1, 6);
    if (coordinator.hasPendingFor("tab-1")) return false;
    if (!coordinator.hasPendingFor("tab-2") || coordinator.pendingRequestId() != 502) return false;

    if (!coordinator.grantTestLease(502, &view2, &webProf, "tab-2", frameC, 1, {"camera"})) return false;
    auto granted = coordinator.grantedKeys(&view2, 1);
    if (!granted.contains("camera")) return false;
    if (!coordinator.grantedKeys(&view2, 2).isEmpty()) return false;

    coordinator.revokeLease(&view2, "camera");
    if (!coordinator.grantedKeys(&view2, 1).isEmpty()) return false;

    qInfo() << "CROSS_ORIGIN_IFRAME_MATRIX_TEST PASS: same/cross origin normalization, screen deny, top-deny cascade, requester-specific isolation, compound evaluation, tab scoping, context expiration, queue priority & epoch invalidation";
    return true;
}

inline bool runPermissionContextTests() {
    if (!runCrossOriginIframeMatrixTests()) return false;
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

    // Test lease granting and soft revocation (preserving view):
    int revokedSignalCount = 0;
    QString lastRevokedKey;
    QObject::connect(&testCoord, &PermissionCoordinator::permissionRevoked, [&](QObject *v, const QString &k) {
        if (v == &viewA) {
            revokedSignalCount++;
            lastRevokedKey = k;
        }
    });

    if (!testCoord.grantTestLease(401, &viewA, &profileA, "tab-a", QUrl("https://a.test"), 1, {"camera", "mic"})) return false;
    if (!testCoord.grantTestLease(402, &viewB, &profileA, "tab-b", QUrl("https://b.test"), 10, {"camera", "notifications"})) return false;

    auto keysA = testCoord.grantedKeys(&viewA, 1);
    if (!keysA.contains("camera") || !keysA.contains("mic") || keysA.size() != 2) return false;
    auto keysB = testCoord.grantedKeys(&viewB, 10);
    if (!keysB.contains("camera") || !keysB.contains("notifications") || keysB.size() != 2) return false;

    // Soft revoke only "camera" on viewA: document and other keys are preserved
    if (!testCoord.revokeLease(&viewA, "camera")) return false;
    if (revokedSignalCount != 1 || lastRevokedKey != "camera") return false;
    keysA = testCoord.grantedKeys(&viewA, 1);
    if (keysA.contains("camera") || !keysA.contains("mic") || keysA.size() != 1) return false;

    // ViewB still has both camera and notifications
    keysB = testCoord.grantedKeys(&viewB, 10);
    if (!keysB.contains("camera") || !keysB.contains("notifications")) return false;

    // Revoke "camera" globally across all views
    testCoord.revokeLease(nullptr, "camera");
    keysB = testCoord.grantedKeys(&viewB, 10);
    if (keysB.contains("camera") || !keysB.contains("notifications") || keysB.size() != 1) return false;

    // Revoke remaining "mic" on viewA
    if (!testCoord.revokeLease(&viewA, "mic")) return false;
    if (revokedSignalCount != 2 || lastRevokedKey != "mic") return false;
    if (!testCoord.grantedKeys(&viewA, 1).isEmpty()) return false;

    // Revoke all remaining leases
    testCoord.revokeAll();
    if (!testCoord.grantedKeys(&viewB, 10).isEmpty()) return false;

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
