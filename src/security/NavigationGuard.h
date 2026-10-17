#pragma once
#include <QUrl>
#include <QString>
#include <QHostAddress>
#include <QStringList>
#include <QChar>
#include <QCoreApplication>

class NavigationGuard {
    Q_DECLARE_TR_FUNCTIONS(NavigationGuard)
public:
    static bool isLocalHost(const QUrl &url) {
        const QString host = url.host().toLower();
        if (host == QStringLiteral("localhost") || host == QStringLiteral("localhost.")
            || host.endsWith(QStringLiteral(".localhost")) || host.endsWith(QStringLiteral(".localhost."))) {
            return true;
        }
        const QHostAddress addr(host);
        if (!addr.isNull() && addr.isLoopback()) {
            return true;
        }
        return false;
    }

    static bool isPrivateNetwork(const QUrl &url) {
        if (isLocalHost(url)) return true;
        QString host = url.host().toLower();
        if (host.startsWith(QLatin1Char('[')) && host.endsWith(QLatin1Char(']'))) {
            host = host.mid(1, host.size() - 2);
        }
        QHostAddress addr(host);
        if (addr.isNull()) return false;

        bool isIpv4Mapped = false;
        const quint32 ipv4 = addr.toIPv4Address(&isIpv4Mapped);
        if (isIpv4Mapped) {
            addr = QHostAddress(ipv4);
        }

        if (addr.isLoopback()) return true;

        // IPv4 private & reserved subnets:
        // 10.0.0.0/8 (RFC 1918)
        // 172.16.0.0/12 (RFC 1918)
        // 192.168.0.0/16 (RFC 1918)
        // 169.254.0.0/16 (RFC 3927 Link-Local / Cloud Metadata)
        // 100.64.0.0/10 (RFC 6598 Shared Address Space / CGNAT)
        // 0.0.0.0/8 (RFC 1122 Current Network)
        if (addr.isInSubnet(QHostAddress(QStringLiteral("10.0.0.0")), 8)
            || addr.isInSubnet(QHostAddress(QStringLiteral("172.16.0.0")), 12)
            || addr.isInSubnet(QHostAddress(QStringLiteral("192.168.0.0")), 16)
            || addr.isInSubnet(QHostAddress(QStringLiteral("169.254.0.0")), 16)
            || addr.isInSubnet(QHostAddress(QStringLiteral("100.64.0.0")), 10)
            || addr.isInSubnet(QHostAddress(QStringLiteral("0.0.0.0")), 8)) {
            return true;
        }

        // IPv6 private & local subnets:
        // fc00::/7 (RFC 4193 Unique Local Address - ULA)
        // fe80::/10 (RFC 4291 Link-Local)
        if (addr.isInSubnet(QHostAddress(QStringLiteral("fc00::")), 7)
            || addr.isInSubnet(QHostAddress(QStringLiteral("fe80::")), 10)) {
            return true;
        }

        return false;
    }

    static bool blocksPrivateNetworkAccess(const QUrl &targetUrl, const QUrl &firstPartyUrl, bool isMainFrame = false) {
        if (!isPrivateNetwork(targetUrl)) {
            return false;
        }

        // Direct user navigation in the main frame with empty or self first-party is allowed
        if (isMainFrame) {
            if (firstPartyUrl.isEmpty() || !firstPartyUrl.isValid() || isPrivateNetwork(firstPartyUrl)) {
                return false;
            }
            // A public first-party site attempting to navigate or redirect to a private address is blocked
            return true;
        }

        // Subresources or embedded subframes:
        // If the first-party is already a private/local network document, internal communication is allowed
        if (isPrivateNetwork(firstPartyUrl)) {
            return false;
        }

        // Public first-party requesting a private target is strictly blocked (PNA protection)
        return true;
    }

    static QString decodePunycodeLabel(const QString &label) {
        if (!label.startsWith(QStringLiteral("xn--"), Qt::CaseInsensitive)) return label;
        const QString ext = label.mid(4);
        int hyphen = ext.lastIndexOf(QLatin1Char('-'));
        QString base;
        QString extended;
        if (hyphen >= 0) {
            base = ext.left(hyphen);
            extended = ext.mid(hyphen + 1).toUpper();
        } else {
            extended = ext.toUpper();
        }

        uint32_t charCode = 0x80;
        int pos = -1;
        int bias = 72;
        int extpos = 0;

        auto adapt = [](int delta, bool first, int numchars) -> int {
            if (first) delta /= 700;
            else delta /= 2;
            delta += delta / numchars;
            int divisions = 0;
            while (delta > 455) {
                delta /= 35;
                divisions += 36;
            }
            return divisions + (36 * delta / (delta + 38));
        };

        auto tFunc = [](int j, int b) -> int {
            int res = 36 * (j + 1) - b;
            if (res < 1) return 1;
            if (res > 26) return 26;
            return res;
        };

        while (extpos < extended.size()) {
            int result = 0;
            int w = 1;
            int j = 0;
            while (true) {
                if (extpos >= extended.size()) return base;
                const QChar ch = extended[extpos++];
                const char latin = ch.toLatin1();
                int digit = 0;
                if (latin >= 'A' && latin <= 'Z') digit = latin - 'A';
                else if (latin >= '0' && latin <= '9') digit = latin - '0' + 26;
                else return base; // Invalid character

                const int t = tFunc(j, bias);
                result += digit * w;
                if (digit < t) break;
                w *= (36 - t);
                ++j;
            }

            pos += result + 1;
            charCode += pos / (base.size() + 1);
            if (charCode > 0x10FFFF) charCode = '?';
            pos = pos % (base.size() + 1);
            base.insert(pos, QChar::fromUcs4(charCode));
            bias = adapt(result, (extpos == 0), base.size());
        }
        return base;
    }

    static bool hasSuspiciousIdn(const QUrl &url) {
        const QString host = url.host().toLower();
        if (!host.contains(QStringLiteral("xn--"))) return false;

        const QStringList labels = host.split(QLatin1Char('.'));
        for (const QString &label : labels) {
            const QString unicodeLabel = decodePunycodeLabel(label);

            bool hasLatin = false;
            bool hasNonLatinConfusable = false;
            for (const QChar &ch : unicodeLabel) {
                const auto s = ch.script();
                if (s == QChar::Script_Latin) {
                    hasLatin = true;
                } else if (s == QChar::Script_Cyrillic || s == QChar::Script_Greek) {
                    hasNonLatinConfusable = true;
                }
            }
            if (hasLatin && hasNonLatinConfusable) {
                return true; // Mixed-script homograph detected
            }
        }
        return false;
    }

    static bool isRestrictedPort(int port) {
        if (port <= 0 || port > 65535) return false;
        if (port == 80 || port == 443 || port == 8080 || port == 8443) return false;
        switch (port) {
        case 1: case 7: case 9: case 11: case 13: case 15: case 17: case 19:
        case 20: case 21: case 22: case 23: case 25: case 37: case 42: case 43:
        case 53: case 77: case 79: case 87: case 95: case 101: case 102: case 103:
        case 104: case 109: case 110: case 111: case 113: case 115: case 117:
        case 119: case 123: case 135: case 139: case 143: case 179: case 389:
        case 445: case 465: case 512: case 513: case 514: case 515: case 526:
        case 530: case 531: case 532: case 540: case 556: case 563: case 587:
        case 601: case 636: case 993: case 995: case 2049: case 3389: case 3659:
        case 4045: case 6000: case 6379: case 6665: case 6666: case 6667:
        case 6668: case 6669: case 11211: case 27017:
            return true;
        default:
            return false;
        }
    }

    static bool requiresRestrictedBrowsing(const QUrl &url) {
        if (!url.isValid() || url.host().isEmpty()) return false;
        if (hasSuspiciousIdn(url)) return true;
        if (url.port() != -1 && isRestrictedPort(url.port())) return true;
        return false;
    }

    static QString restrictedBrowsingReason(const QUrl &url) {
        if (hasSuspiciousIdn(url)) {
            return tr("Attaque homographique IDN détectée : mélange de caractères alphabétiques suspects.");
        }
        if (url.port() != -1 && isRestrictedPort(url.port())) {
            return tr("Port réseau dangereux ou restreint.");
        }
        return {};
    }
};
