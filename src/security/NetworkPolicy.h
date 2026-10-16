#pragma once
#include <QUrl>

namespace NetworkPolicy {
// The strict mode has no implicit exceptions, including localhost and LAN hosts.
inline bool blocks(const QUrl &url, bool httpsOnly) {
    return httpsOnly && (url.scheme() == "http" || url.scheme() == "ws");
}
inline bool hasWebCredentials(const QUrl &url) {
    return (url.scheme() == "http" || url.scheme() == "https")
        && (!url.userName().isEmpty() || !url.password().isEmpty());
}
}
