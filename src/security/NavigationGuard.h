#pragma once
#include <QUrl>
#include <QString>
#include <QHostAddress>
#include <QStringList>
#include <QChar>
#include <QCoreApplication>
#include <QMutex>
#include <QHash>
#include <QDebug>

class NavigationGuard {
    Q_DECLARE_TR_FUNCTIONS(NavigationGuard)
public:
    // TLDs and domain suffixes reserved for local / private networks
    static bool isSpecialPrivateDomain(const QString &host) {
        if (host.isEmpty()) return false;
        const QString h = host.toLower();

        // RFC 6761 .localhost
        if (h == QStringLiteral("localhost") || h == QStringLiteral("localhost.")
            || h.endsWith(QStringLiteral(".localhost")) || h.endsWith(QStringLiteral(".localhost."))) {
            return true;
        }

        // RFC 6762 Multicast DNS .local
        if (h == QStringLiteral("local") || h == QStringLiteral("local.")
            || h.endsWith(QStringLiteral(".local")) || h.endsWith(QStringLiteral(".local."))) {
            return true;
        }

        // RFC 8375 Special-Use Domain for Home Networks .home.arpa
        if (h.endsWith(QStringLiteral(".home.arpa")) || h.endsWith(QStringLiteral(".home.arpa."))) {
            return true;
        }

        // Standard private LAN suffixes commonly used in corporate and home networks
        if (h.endsWith(QStringLiteral(".lan")) || h.endsWith(QStringLiteral(".lan."))
            || h.endsWith(QStringLiteral(".internal")) || h.endsWith(QStringLiteral(".internal."))
            || h.endsWith(QStringLiteral(".corp")) || h.endsWith(QStringLiteral(".corp."))
            || h.endsWith(QStringLiteral(".home")) || h.endsWith(QStringLiteral(".home."))
            || h.endsWith(QStringLiteral(".router")) || h.endsWith(QStringLiteral(".router."))) {
            return true;
        }

        // Canonical default gateway hostnames for popular home routers
        static const QStringList knownRouterHosts = {
            QStringLiteral("fritz.box"), QStringLiteral("speedport.ip"),
            QStringLiteral("my.router"), QStringLiteral("openwrt.lan"),
            QStringLiteral("tplinkwifi.net"), QStringLiteral("routerlogin.net"),
            QStringLiteral("routerlogin.com"), QStringLiteral("tendawifi.com")
        };
        if (knownRouterHosts.contains(h)) {
            return true;
        }

        return false;
    }

    // Robust parser for alternative IPv4 representations (dword, hex, octal, 2-part and 3-part dotted notation)
    static bool tryParseIpv4(const QString &host, QHostAddress &outAddr) {
        if (host.isEmpty()) return false;
        QString clean = host.trimmed().toLower();
        if (clean.isEmpty()) return false;

        bool ok = false;
        // Case 1: Dotless integer notation (e.g. 2130706433, 0x7f000001, 017700000001)
        if (!clean.contains(QLatin1Char('.'))) {
            const qulonglong val = clean.toULongLong(&ok, 0);
            if (ok && val <= 0xFFFFFFFFULL) {
                outAddr = QHostAddress(static_cast<quint32>(val));
                return true;
            }
            return false;
        }

        // Case 2: Dotted notation with 2, 3, or 4 parts (supports decimal, hex 0x, octal 0)
        const QStringList parts = clean.split(QLatin1Char('.'));
        if (parts.size() < 2 || parts.size() > 4) return false;

        quint32 parsedParts[4] = {0, 0, 0, 0};
        for (int i = 0; i < parts.size(); ++i) {
            const QString &p = parts[i];
            if (p.isEmpty()) return false;
            const qulonglong v = p.toULongLong(&ok, 0);
            if (!ok) return false;

            if (i < parts.size() - 1) {
                if (v > 255) return false;
            } else {
                const quint64 maxVal = (parts.size() == 4) ? 255ULL :
                                       (parts.size() == 3) ? 65535ULL : 16777215ULL;
                if (v > maxVal) return false;
            }
            parsedParts[i] = static_cast<quint32>(v);
        }

        quint32 ipv4 = 0;
        if (parts.size() == 4) {
            ipv4 = (parsedParts[0] << 24) | (parsedParts[1] << 16) | (parsedParts[2] << 8) | parsedParts[3];
        } else if (parts.size() == 3) {
            ipv4 = (parsedParts[0] << 24) | (parsedParts[1] << 16) | parsedParts[2];
        } else if (parts.size() == 2) {
            ipv4 = (parsedParts[0] << 24) | parsedParts[1];
        }

        outAddr = QHostAddress(ipv4);
        return true;
    }

    static bool isPrivateNetworkAddress(const QHostAddress &addr) {
        if (addr.isNull()) return false;
        if (addr.isLoopback()) return true;
        if (addr == QHostAddress::AnyIPv4 || addr == QHostAddress::AnyIPv6
            || addr == QHostAddress(QStringLiteral("0.0.0.0"))
            || addr == QHostAddress(QStringLiteral("::"))) {
            return true;
        }

        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            // 10.0.0.0/8 (RFC 1918)
            // 172.16.0.0/12 (RFC 1918)
            // 192.168.0.0/16 (RFC 1918)
            // 169.254.0.0/16 (RFC 3927 Link-Local / Cloud Metadata 169.254.169.254)
            // 100.64.0.0/10 (RFC 6598 Shared Address Space / CGNAT)
            // 0.0.0.0/8 (RFC 1122 Current Network)
            // 127.0.0.0/8 (RFC 1122 Loopback)
            // 224.0.0.0/4 (RFC 5771 Multicast)
            // 240.0.0.0/4 (RFC 1122 Reserved / Future use)
            // 255.255.255.255/32 (Limited Broadcast)
            if (addr.isInSubnet(QHostAddress(QStringLiteral("10.0.0.0")), 8)
                || addr.isInSubnet(QHostAddress(QStringLiteral("172.16.0.0")), 12)
                || addr.isInSubnet(QHostAddress(QStringLiteral("192.168.0.0")), 16)
                || addr.isInSubnet(QHostAddress(QStringLiteral("169.254.0.0")), 16)
                || addr.isInSubnet(QHostAddress(QStringLiteral("100.64.0.0")), 10)
                || addr.isInSubnet(QHostAddress(QStringLiteral("0.0.0.0")), 8)
                || addr.isInSubnet(QHostAddress(QStringLiteral("127.0.0.0")), 8)
                || addr.isInSubnet(QHostAddress(QStringLiteral("224.0.0.0")), 4)
                || addr.isInSubnet(QHostAddress(QStringLiteral("240.0.0.0")), 4)
                || addr == QHostAddress(QStringLiteral("255.255.255.255"))) {
                return true;
            }
            return false;
        }

        if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
            // Handle IPv4-compatible (::x.x.x.x) or IPv4-mapped (::ffff:x.x.x.x)
            const Q_IPV6ADDR ipv6 = addr.toIPv6Address();
            bool isZeros = true;
            for (int i = 0; i < 10; ++i) {
                if (ipv6[i] != 0) { isZeros = false; break; }
            }
            if (isZeros) {
                const bool isMapped = (ipv6[10] == 0xff && ipv6[11] == 0xff);
                const bool isCompatible = (ipv6[10] == 0x00 && ipv6[11] == 0x00);
                if (isMapped || isCompatible) {
                    const quint32 ipv4 = (static_cast<quint32>(ipv6[12]) << 24)
                                       | (static_cast<quint32>(ipv6[13]) << 16)
                                       | (static_cast<quint32>(ipv6[14]) << 8)
                                       | static_cast<quint32>(ipv6[15]);
                    if (ipv4 > 1 || isMapped) {
                        return isPrivateNetworkAddress(QHostAddress(ipv4));
                    }
                }
            }

            // fc00::/7 (RFC 4193 Unique Local Address - ULA)
            // fe80::/10 (RFC 4291 Link-Local)
            // fec0::/10 (RFC 3879 Site-Local deprecated)
            // ff00::/8 (RFC 4291 Multicast)
            if (addr.isInSubnet(QHostAddress(QStringLiteral("fc00::")), 7)
                || addr.isInSubnet(QHostAddress(QStringLiteral("fe80::")), 10)
                || addr.isInSubnet(QHostAddress(QStringLiteral("fec0::")), 10)
                || addr.isInSubnet(QHostAddress(QStringLiteral("ff00::")), 8)) {
                return true;
            }
            return false;
        }

        return false;
    }

    static bool isLocalHost(const QUrl &url) {
        const QString host = url.host().toLower();
        if (host == QStringLiteral("localhost") || host == QStringLiteral("localhost.")
            || host.endsWith(QStringLiteral(".localhost")) || host.endsWith(QStringLiteral(".localhost."))) {
            return true;
        }

        QHostAddress addr;
        if (tryParseIpv4(host, addr)) {
            return addr.isLoopback();
        }

        QString cleanHost = host;
        if (cleanHost.startsWith(QLatin1Char('[')) && cleanHost.endsWith(QLatin1Char(']'))) {
            cleanHost = cleanHost.mid(1, cleanHost.size() - 2);
        }
        if (addr.setAddress(cleanHost)) {
            if (addr.isLoopback()) return true;
            if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
                const Q_IPV6ADDR ipv6 = addr.toIPv6Address();
                bool isZeros = true;
                for (int i = 0; i < 10; ++i) {
                    if (ipv6[i] != 0) { isZeros = false; break; }
                }
                if (isZeros) {
                    const bool isMapped = (ipv6[10] == 0xff && ipv6[11] == 0xff);
                    const bool isCompatible = (ipv6[10] == 0x00 && ipv6[11] == 0x00);
                    if (isMapped || isCompatible) {
                        const quint32 ipv4 = (static_cast<quint32>(ipv6[12]) << 24)
                                           | (static_cast<quint32>(ipv6[13]) << 16)
                                           | (static_cast<quint32>(ipv6[14]) << 8)
                                           | static_cast<quint32>(ipv6[15]);
                        return QHostAddress(ipv4).isLoopback();
                    }
                }
            }
        }
        return false;
    }

    static bool isPrivateNetwork(const QUrl &url) {
        if (!url.isValid()) return false;
        const QString host = url.host().toLower();
        if (host.isEmpty()) return false;

        if (isLocalHost(url)) return true;
        if (isSpecialPrivateDomain(host)) return true;

        QHostAddress addr;
        if (tryParseIpv4(host, addr)) {
            return isPrivateNetworkAddress(addr);
        }

        QString cleanHost = host;
        if (cleanHost.startsWith(QLatin1Char('[')) && cleanHost.endsWith(QLatin1Char(']'))) {
            cleanHost = cleanHost.mid(1, cleanHost.size() - 2);
        }
        if (addr.setAddress(cleanHost)) {
            return isPrivateNetworkAddress(addr);
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

    // DNS Rebinding Guard: Tracks host-to-scope observations to detect and block DNS rebinding attacks
    class DnsRebindGuard {
    public:
        enum AddressScope {
            ScopeUnknown,
            ScopePublic,
            ScopePrivate
        };

        static DnsRebindGuard &instance() {
            static DnsRebindGuard s_instance;
            return s_instance;
        }

        bool recordResolution(const QString &hostName, const QHostAddress &addr, const QUrl &firstPartyUrl) {
            const QString host = hostName.toLower();
            if (host.isEmpty() || addr.isNull()) return true;

            const AddressScope currentScope = NavigationGuard::isPrivateNetworkAddress(addr) ? ScopePrivate : ScopePublic;
            const bool isFirstPartyPublic = !firstPartyUrl.isEmpty() && !NavigationGuard::isPrivateNetwork(firstPartyUrl);

            QMutexLocker locker(&m_mutex);
            auto it = m_hostScopes.find(host);
            if (it != m_hostScopes.end()) {
                if (it.value() == ScopePublic && currentScope == ScopePrivate) {
                    if (isFirstPartyPublic) {
                        qWarning() << "SECURITY: DNS rebinding detected for host" << host
                                   << "to private IP" << addr.toString()
                                   << "from public first-party" << firstPartyUrl.toString();
                        return false; // REBINDING BLOCKED
                    }
                }
            }
            m_hostScopes[host] = currentScope;
            return true;
        }

        void reset() {
            QMutexLocker locker(&m_mutex);
            m_hostScopes.clear();
        }

    private:
        QMutex m_mutex;
        QHash<QString, AddressScope> m_hostScopes;
    };

    static bool checkDnsRebind(const QString &hostName, const QHostAddress &addr, const QUrl &firstPartyUrl) {
        return DnsRebindGuard::instance().recordResolution(hostName, addr, firstPartyUrl);
    }

    static void resetDnsRebindState() {
        DnsRebindGuard::instance().reset();
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
