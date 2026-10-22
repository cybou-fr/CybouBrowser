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
// Strict WebRTC (on by default since 0.3.0-alpha.1, user can turn it off): disable_non_proxied_udp forbids direct UDP, which is the
// only measured way to stop a page from making the browser send STUN checks to loopback/private
// addresses via injected ICE candidates. It also removes direct peer-to-peer media, so calls rely
// on TCP/TLS relays. The switch is process-wide, hence it takes effect after a restart.
// Value used when the user has never chosen: the saved preference "preferences/strictWebRtc" overrides it.
inline constexpr bool kStrictWebRtcDefault = true;
inline constexpr const char *kStrictWebRtcSwitch = "--force-webrtc-ip-handling-policy=disable_non_proxied_udp";
//
// WebAssembly off (on by default since 0.4.0-alpha.1, user can turn it off): a stopgap for CVE-2026-87491, a V8 bug
// exploited in the wild and reachable only from WebAssembly, which the Qt WebEngine shipped with Qt 6.12.0 does not fix
// (tests/security/vulns/triage.json). The V8 in Qt 6.12 no longer has --expose-wasm (an unknown flag is ignored, as
// --wasm-test showed); it hides the WebAssembly global only in --jitless mode. Jitless also turns off the JavaScript JIT:
// pages run slower, sites that need WebAssembly stop working, and JIT compiler bugs become unreachable as well.
// Process-wide like the WebRTC switch: a change applies after a restart. Remove the default once a
// Qt WebEngine release carries the fix. Saved preference: "preferences/disableWasm".
inline constexpr bool kDisableWasmDefault = true;
inline constexpr const char *kDisableWasmSwitch = "--js-flags=--jitless";

// Pure so the flag composition can be tested without touching the environment.
inline QByteArray engineHardeningFlags(QByteArray flags, bool strictWebRtc = false, bool disableWasm = false) {
    const QByteArray feature = "BlockInsecurePrivateNetworkRequests,PrivateNetworkAccessSendPreflights,PrivateNetworkAccessRespectPreflightResults";
    const QByteArray key = "--enable-features=";
    const int at = flags.indexOf(key);
    if (at < 0) flags += " " + key + feature;
    else if (!flags.contains("PrivateNetworkAccessRespectPreflightResults")) flags.insert(at + key.size(), feature + ",");
    // An explicitly supplied policy always wins over the preference.
    if (strictWebRtc && !flags.contains("--force-webrtc-ip-handling-policy")) flags += QByteArray(" ") + kStrictWebRtcSwitch;
    // Chromium takes one --js-flags; an explicitly supplied one wins, as for the WebRTC policy.
    if (disableWasm && !flags.contains("--js-flags")) flags += QByteArray(" ") + kDisableWasmSwitch;
    return flags;
}

inline void applyEngineHardening(bool strictWebRtc = false, bool disableWasm = false) {
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", engineHardeningFlags(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS"), strictWebRtc, disableWasm));
}
