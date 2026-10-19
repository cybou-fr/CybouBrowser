#pragma once
#include <QChar>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

// Describes a link that would hand control to an external application (mailto:, tel:, zoommtg:, ...).
// The browser never launches such applications (unknown URL schemes are disallowed), so the user is
// told why nothing happens. Everything shown comes from the page and is therefore cleaned: control and
// format characters (including right-to-left overrides) are dropped, whitespace is collapsed and the
// length is bounded. The UI must still render it as plain text.
namespace ExternalLinkNotice {

inline bool isEngineScheme(const QString &scheme) {
    static const QStringList known{"http", "https", "file", "about", "data", "blob", "javascript", "view-source",
                                   "ftp", "ws", "wss", "filesystem", "qrc", "chrome", "devtools"};
    return known.contains(scheme);
}

inline QString cleaned(const QString &text, int limit, const QString &allowed = QString()) {
    QString out;
    bool pendingSpace = false;
    for (const QChar ch : text) {
        if (ch.isSpace()) { pendingSpace = !out.isEmpty(); continue; }
        const auto category = ch.category();
        if (category == QChar::Other_Control || category == QChar::Other_Format || category == QChar::Other_Surrogate
            || category == QChar::Other_PrivateUse || category == QChar::Other_NotAssigned) continue;
        if (!allowed.isEmpty() && !ch.isDigit() && !allowed.contains(ch)) continue;
        if (pendingSpace) { out += QLatin1Char(' '); pendingSpace = false; }
        out += ch;
        if (out.size() >= limit) return out + QChar(0x2026);
    }
    return out;
}

// Empty map: not an external-application link. Otherwise {scheme, kind: mail|phone|other, target}.
inline QVariantMap describe(const QUrl &url) {
    const QString scheme = url.scheme().toLower();
    if (scheme.isEmpty() || isEngineScheme(scheme)) return {};
    QVariantMap info;
    info.insert("scheme", cleaned(scheme, 32, QStringLiteral("abcdefghijklmnopqrstuvwxyz+.-")));
    const QString path = url.path(QUrl::FullyDecoded);
    if (scheme == QLatin1String("mailto")) {
        info.insert("kind", "mail");
        info.insert("target", cleaned(path, 120));
    } else if (scheme == QLatin1String("tel") || scheme == QLatin1String("sms") || scheme == QLatin1String("smsto") || scheme == QLatin1String("callto")) {
        info.insert("kind", "phone");
        info.insert("target", cleaned(path, 40, QStringLiteral("+()-.#*;,")));
    } else {
        info.insert("kind", "other");
        info.insert("target", QString());
    }
    return info;
}

} // namespace ExternalLinkNotice
