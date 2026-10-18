#pragma once
#include <QByteArray>
#include <QtGlobal>

// Chromium switches applied before QtWebEngineQuick::initialize(). Shared by the product and
// cybou_tests so tests exercise the shipped engine configuration.
// BlockInsecurePrivateNetworkRequests: an insecure-context page in a public address space cannot
// reach private/local addresses. Enforced on the connected address, so it also covers hostnames
// that resolve to private IPs (see docs/VALIDATION.md, engine-level experiment).
inline void applyEngineHardening() {
    QByteArray flags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    const QByteArray feature = "BlockInsecurePrivateNetworkRequests";
    const QByteArray key = "--enable-features=";
    const int at = flags.indexOf(key);
    if (at < 0) flags += " " + key + feature;
    else if (!flags.contains(feature)) flags.insert(at + key.size(), feature + ",");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags);
}
