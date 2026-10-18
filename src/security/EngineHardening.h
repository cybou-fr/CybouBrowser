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
inline void applyEngineHardening() {
    QByteArray flags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    const QByteArray feature = "BlockInsecurePrivateNetworkRequests,PrivateNetworkAccessSendPreflights,PrivateNetworkAccessRespectPreflightResults";
    const QByteArray key = "--enable-features=";
    const int at = flags.indexOf(key);
    if (at < 0) flags += " " + key + feature;
    else if (!flags.contains("PrivateNetworkAccessRespectPreflightResults")) flags.insert(at + key.size(), feature + ",");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags);
}
