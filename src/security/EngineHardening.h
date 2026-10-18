#pragma once
#include <QByteArray>
#include <QtGlobal>

// Chromium switches applied before QtWebEngineQuick::initialize(). Shared by the product and
// cybou_tests so tests exercise the shipped engine configuration.
// BlockInsecurePrivateNetworkRequests: an insecure-context page in a public address space cannot
// reach private/local addresses. PrivateNetworkAccessSendPreflights + ...RespectPreflightResults:
// a secure public page may reach them only if the target answers a preflight with
// Access-Control-Allow-Private-Network. Both are enforced on the connected address, so they also
// cover hostnames that resolve to private IPs (see docs/VALIDATION.md, engine-level experiments).
//
// Strict WebRTC (opt-in, off by default): disable_non_proxied_udp forbids direct UDP, which is the
// only measured way to stop a page from making the browser send STUN checks to loopback/private
// addresses via injected ICE candidates. It also removes direct peer-to-peer media, so calls rely
// on TCP/TLS relays. The switch is process-wide, hence it takes effect after a restart.
inline constexpr const char *kStrictWebRtcSwitch = "--force-webrtc-ip-handling-policy=disable_non_proxied_udp";

// Pure so the flag composition can be tested without touching the environment.
inline QByteArray engineHardeningFlags(QByteArray flags, bool strictWebRtc = false) {
    const QByteArray feature = "BlockInsecurePrivateNetworkRequests,PrivateNetworkAccessSendPreflights,PrivateNetworkAccessRespectPreflightResults";
    const QByteArray key = "--enable-features=";
    const int at = flags.indexOf(key);
    if (at < 0) flags += " " + key + feature;
    else if (!flags.contains("PrivateNetworkAccessRespectPreflightResults")) flags.insert(at + key.size(), feature + ",");
    // An explicitly supplied policy always wins over the preference.
    if (strictWebRtc && !flags.contains("--force-webrtc-ip-handling-policy")) flags += QByteArray(" ") + kStrictWebRtcSwitch;
    return flags;
}

inline void applyEngineHardening(bool strictWebRtc = false) {
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", engineHardeningFlags(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS"), strictWebRtc));
}
