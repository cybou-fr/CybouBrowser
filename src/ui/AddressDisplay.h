#pragma once
#include <QUrl>
#include <QFileInfo>

namespace AddressDisplay {
inline QString host(const QUrl &url) {
    const auto scheme = url.scheme().toLower();
    if (scheme == "http" || scheme == "https") {
        auto name = url.host();
        if (name.isEmpty()) return scheme + ":";
        // Display the complete ACE hostname: no home-grown public suffix rules,
        // user-info, Unicode confusables, path or query in the compact label.
        name = name.contains(':') ? "[" + name + "]" : QString::fromLatin1(QUrl::toAce(name));
        const int port = url.port();
        if (port >= 0 && port != (scheme == "https" ? 443 : 80)) name += ":" + QString::number(port);
        return name;
    }
    if (url.isLocalFile()) return "file: " + QFileInfo(url.toLocalFile()).fileName();
    return scheme == "about" && url.path() == "blank" ? QString() : scheme + ":";
}
}
