#pragma once
#include <QCoreApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include "EngineHardening.h"

// Pure flag-composition checks for the opt-in strict WebRTC switch.
inline bool runEngineHardeningFlagTests() {
    const QByteArray strict = kStrictWebRtcSwitch;
    const auto off = engineHardeningFlags("");
    if (off.contains("--force-webrtc-ip-handling-policy")) { qCritical() << "STRICT_WEBRTC FAIL: policy present while disabled"; return false; }
    if (!off.contains("BlockInsecurePrivateNetworkRequests")) { qCritical() << "STRICT_WEBRTC FAIL: PNA hardening missing"; return false; }
    const auto on = engineHardeningFlags("", true);
    if (on.count(strict) != 1 || !on.contains("BlockInsecurePrivateNetworkRequests")) { qCritical() << "STRICT_WEBRTC FAIL: enabled flags" << on; return false; }
    const auto twice = engineHardeningFlags(on, true);
    if (twice.count(strict) != 1 || twice.count("--enable-features=") != 1) { qCritical() << "STRICT_WEBRTC FAIL: not idempotent" << twice; return false; }
    const auto merged = engineHardeningFlags("--enable-features=Foo --disable-gpu", true);
    if (!merged.contains("--enable-features=BlockInsecurePrivateNetworkRequests") || !merged.contains(",Foo") || !merged.contains("--disable-gpu") || merged.count(strict) != 1) {
        qCritical() << "STRICT_WEBRTC FAIL: existing flags not preserved" << merged; return false;
    }
    const auto explicitPolicy = engineHardeningFlags("--force-webrtc-ip-handling-policy=default_public_interface_only", true);
    if (explicitPolicy.contains("disable_non_proxied_udp")) { qCritical() << "STRICT_WEBRTC FAIL: explicit policy overridden"; return false; }
    qInfo() << "STRICT_WEBRTC_FLAGS PASS: off by default, single switch when enabled, idempotent, merges and respects explicit policy";
    return true;
}

// Parent: runs this executable twice in child mode (strict off = control, strict on) because the
// switch is process-wide. Each child reports how many STUN checks reached a loopback UDP socket
// after the page injected a loopback ICE candidate.
inline int runStrictWebRtcTest() {
    if (!runEngineHardeningFlagTests()) return 1;
    int counts[2] = {-1, -1};
    for (int strict : {0, 1}) {
        QProcess child;
        child.setProcessChannelMode(QProcess::MergedChannels);
        child.start(QCoreApplication::applicationFilePath(), {QString("--webrtc-strict-child=%1").arg(strict)});
        if (!child.waitForFinished(240000)) { child.kill(); child.waitForFinished(5000); qCritical() << "STRICT_WEBRTC FAIL: child timeout" << strict; return 1; }
        const QString output = QString::fromUtf8(child.readAll());
        const auto match = QRegularExpression("stunChecksAtLoopback (\\d+)").match(output);
        if (child.exitStatus() != QProcess::NormalExit || child.exitCode() != 0 || !match.hasMatch()) {
            qCritical().noquote() << "STRICT_WEBRTC FAIL: child" << strict << "exit" << child.exitCode() << "output tail:" << output.right(600);
            return 1;
        }
        counts[strict] = match.captured(1).toInt();
        qInfo() << "STRICT_WEBRTC_CHILD" << (strict ? "strict" : "control") << "stunChecksAtLoopback" << counts[strict];
    }
    if (counts[0] < 10) { qCritical() << "STRICT_WEBRTC FAIL: control produced too few checks to prove the vector" << counts[0]; return 1; }
    if (counts[1] != 0) { qCritical() << "STRICT_WEBRTC FAIL: strict mode still sent checks" << counts[1]; return 1; }
    qInfo() << "STRICT_WEBRTC_TEST PASS: control reached loopback with" << counts[0] << "STUN checks; strict mode sent 0";
    return 0;
}
