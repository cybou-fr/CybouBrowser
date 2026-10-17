#pragma once
#include "NavigationGuard.h"
#include <QDebug>

inline bool runNavigationGuardTests() {
    // 1. Loopback addresses (standard and IPv6)
    for (const auto &urlStr : {"http://127.0.0.1/", "http://127.0.0.5:8080/", "http://127.255.255.255/",
                               "http://localhost/", "http://test.localhost/", "http://[::1]/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isLocalHost(url) || !NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: loopback classification" << urlStr;
            return false;
        }
    }

    // 2. Alternative IPv4 notation formats (DWORD, Hex, Octal, 2-part and 3-part dotted notation)
    // Used by SSRF / DNS rebinding / PNA bypass exploits
    for (const auto &urlStr : {"http://2130706433/",             // 127.0.0.1 in decimal DWORD
                               "http://0x7f000001/",            // 127.0.0.1 in hex DWORD
                               "http://0x7f.0.0.1/",            // 127.0.0.1 in hex dotted
                               "http://0x7f.1/",                // 127.0.0.1 in hex 2-part dotted
                               "http://017700000001/",          // 127.0.0.1 in octal DWORD
                               "http://0177.0.0.1/",            // 127.0.0.1 in octal dotted
                               "http://127.1/",                 // 127.0.0.1 in 2-part dotted
                               "http://127.0.1/",               // 127.0.0.1 in 3-part dotted
                               "http://3232235521/",            // 192.168.0.1 in decimal DWORD
                               "http://0xc0a80001/",            // 192.168.0.1 in hex DWORD
                               "http://192.168.1/",             // 192.168.0.1 in 3-part dotted
                               "http://167772161/",             // 10.0.0.1 in decimal DWORD
                               "http://10.1/",                  // 10.0.0.1 in 2-part dotted
                               "http://2886729729/"}) {         // 172.16.0.1 in decimal DWORD
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: alternative IPv4 format classification" << urlStr;
            return false;
        }
    }

    // 3. Private IPv4 addresses (RFC 1918, RFC 3927 link-local, RFC 6598 CGNAT, RFC 1122, multicast & broadcast)
    for (const auto &urlStr : {"http://10.1.2.3/", "http://172.16.5.6/", "http://172.31.255.254/",
                               "http://192.168.1.1/", "http://169.254.169.254/", "http://169.254.10.20/",
                               "http://100.64.0.1/", "http://0.0.0.0/",
                               "http://224.0.0.1/", "http://239.255.255.250/", "http://255.255.255.255/", "http://240.0.0.1/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: private IPv4 classification" << urlStr;
            return false;
        }
    }

    // 4. Private IPv6 addresses (ULA fc00::/7, Link-Local fe80::/10, Site-Local fec0::/10, Multicast ff00::/8, IPv4-mapped & compatible)
    for (const auto &urlStr : {"http://[fc00::1]/", "http://[fd12:3456:789a::1]/", "http://[fe80::1]/", "http://[fec0::1]/",
                               "http://[ff02::1]/", "http://[::]/",
                               "http://[::ffff:192.168.1.1]/", "http://[::ffff:127.0.0.1]/", "http://[::ffff:10.0.0.1]/",
                               "http://[::127.0.0.1]/", "http://[::192.168.1.1]/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: private IPv6 classification" << urlStr;
            return false;
        }
    }

    // 5. Special-use private and local network domains (RFC 6761, RFC 6762 mDNS, RFC 8375, home routers)
    for (const auto &urlStr : {"http://router.local/", "http://printer.local./",
                               "http://gateway.home.arpa/", "http://nas.home.arpa./",
                               "http://server.lan/", "http://switch.lan./",
                               "http://db.internal/", "http://service.corp/",
                               "http://fritz.box/", "http://speedport.ip/", "http://my.router/",
                               "http://openwrt.lan/", "http://tplinkwifi.net/", "http://routerlogin.net/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: special private domain classification" << urlStr;
            return false;
        }
    }

    // 6. Public addresses (MUST NOT be classified as private network)
    for (const auto &urlStr : {"https://8.8.8.8/", "https://1.1.1.1/", "https://142.250.190.46/",
                               "https://172.15.255.255/", "https://172.32.0.1/",
                               "https://example.com/", "https://cybou.fr/", "https://proton.me/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: public classified as private" << urlStr;
            return false;
        }
    }

    // 7. Cross-network Private Network Access (PNA) protection
    const QUrl publicFirstParty("https://attacker.example/page");
    const QUrl internalFirstParty("http://192.168.1.10:8080/dashboard");
    const QUrl localFirstParty("http://localhost:3000/app");

    // Public -> Private subresources: MUST be blocked
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/admin"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 192.168.1.1 subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://127.0.0.1:8080/api"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 127.0.0.1 subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://localhost:9000/"), publicFirstParty, false)) { qCritical() << "PNA FAIL: localhost subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://169.254.169.254/latest/meta-data/"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 169.254 subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://10.0.0.5/secret"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 10.0.0.5 subresource"; return false; }

    // Public -> Alternative IPv4 format targets: MUST be blocked
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://2130706433/secret"), publicFirstParty, false)) { qCritical() << "PNA FAIL: dword loopback subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://0x7f000001/api"), publicFirstParty, false)) { qCritical() << "PNA FAIL: hex loopback subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://127.1/token"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 127.1 subresource"; return false; }

    // Public -> Local special domain targets: MUST be blocked
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://router.local/api"), publicFirstParty, false)) { qCritical() << "PNA FAIL: router.local subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://fritz.box/status"), publicFirstParty, false)) { qCritical() << "PNA FAIL: fritz.box subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://gateway.home.arpa/reboot"), publicFirstParty, false)) { qCritical() << "PNA FAIL: home.arpa subresource"; return false; }

    // Public -> Private mainFrame navigation attempt: MUST be blocked
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/reboot"), publicFirstParty, true)) { qCritical() << "PNA FAIL: 192.168.1.1 mainFrame"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://router.local/reboot"), publicFirstParty, true)) { qCritical() << "PNA FAIL: router.local mainFrame"; return false; }

    // Internal -> Internal: MUST be allowed
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/api"), internalFirstParty, false)) { qCritical() << "PNA FAIL: internal->internal"; return false; }
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://localhost:3000/api"), localFirstParty, false)) { qCritical() << "PNA FAIL: local->local"; return false; }

    // Direct address bar navigation (empty first-party): MUST be allowed
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/"), QUrl(), true)) { qCritical() << "PNA FAIL: direct 192.168.1.1"; return false; }
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://localhost:8080/"), QUrl(), true)) { qCritical() << "PNA FAIL: direct localhost"; return false; }
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://router.local/"), QUrl(), true)) { qCritical() << "PNA FAIL: direct router.local"; return false; }

    // Public -> Public: MUST be allowed
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("https://cdn.example/lib.js"), publicFirstParty, false)) { qCritical() << "PNA FAIL: public->public"; return false; }

    // 8. DNS Rebinding detection and prevention
    NavigationGuard::resetDnsRebindState();
    const QString rebindHost("rebind.attacker.example");
    const QHostAddress publicIp("93.184.216.34");
    const QHostAddress privateIp("127.0.0.1");

    // First resolution: domain resolves to public IP -> Allowed
    if (!NavigationGuard::checkDnsRebind(rebindHost, publicIp, publicFirstParty)) {
        qCritical() << "DNS_REBIND FAIL: initial public IP resolution rejected";
        return false;
    }
    // Same domain resolves again to public IP -> Allowed
    if (!NavigationGuard::checkDnsRebind(rebindHost, QHostAddress("93.184.216.35"), publicFirstParty)) {
        qCritical() << "DNS_REBIND FAIL: subsequent public IP resolution rejected";
        return false;
    }
    // Attacker rebinds domain to loopback / private IP from public first-party -> MUST be blocked!
    if (NavigationGuard::checkDnsRebind(rebindHost, privateIp, publicFirstParty)) {
        qCritical() << "DNS_REBIND FAIL: rebinding from public to private IP was not blocked!";
        return false;
    }

    // 9. Suspicious IDN / Homograph detection
    if (NavigationGuard::hasSuspiciousIdn(QUrl("https://example.com/"))) { qCritical() << "IDN FAIL: example.com"; return false; }
    if (NavigationGuard::hasSuspiciousIdn(QUrl("https://xn--bcher-kva.example/"))) { qCritical() << "IDN FAIL: bücher.example"; return false; }
    if (NavigationGuard::hasSuspiciousIdn(QUrl("https://xn--e1afmkfd.xn--p1ai/"))) { qCritical() << "IDN FAIL: пример.рф"; return false; }
    if (!NavigationGuard::hasSuspiciousIdn(QUrl("https://xn--pple-43d.example/"))) { qCritical() << "IDN FAIL: аpple.example"; return false; }

    // 10. Restricted ports (IANA / Chromium dangerous ports blocked for general web browsing)
    const int dangerousPorts[] = { 1, 7, 9, 11, 13, 15, 17, 19, 20, 21, 22, 23, 25, 37, 42, 43, 53, 77, 79, 87, 95,
                                   101, 102, 103, 104, 109, 110, 111, 113, 115, 117, 119, 123, 135, 139, 143, 179,
                                   389, 445, 465, 512, 513, 514, 515, 526, 530, 531, 532, 540, 556, 563, 587, 601,
                                   636, 993, 995, 2049, 3389, 3659, 4045, 6000, 6379, 6665, 6666, 6667, 6668, 6669,
                                   11211, 27017 };
    for (int p : dangerousPorts) {
        if (!NavigationGuard::isRestrictedPort(p)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: dangerous port not restricted" << p;
            return false;
        }
    }
    const int standardPorts[] = { 80, 443, 8080, 8443, 3000, 5000, 8000, 9000 };
    for (int p : standardPorts) {
        if (NavigationGuard::isRestrictedPort(p)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: safe port flagged as restricted" << p;
            return false;
        }
    }
    if (NavigationGuard::isRestrictedPort(-1) || NavigationGuard::isRestrictedPort(0) || NavigationGuard::isRestrictedPort(70000)) {
        qCritical() << "NAVIGATION_GUARD_TEST FAIL: out of range port flagged";
        return false;
    }

    // 11. Restricted Browsing trigger and reason
    const QUrl homographUrl("https://xn--pple-43d.example/");
    if (!NavigationGuard::requiresRestrictedBrowsing(homographUrl)) {
        qCritical() << "RESTRICTED_BROWSING_TEST FAIL: homograph URL not flagged";
        return false;
    }
    const QString homographReason = NavigationGuard::restrictedBrowsingReason(homographUrl);
    if (homographReason.isEmpty() || !homographReason.contains("homographique")) {
        qCritical() << "RESTRICTED_BROWSING_TEST FAIL: homograph reason unexpected" << homographReason;
        return false;
    }

    // Whole-script confusable spoof: all-Cyrillic 'apple.com' under Latin .com TLD
    const QUrl wholeScriptSpoofUrl("https://xn--80ak6aa92e.com/");
    if (!NavigationGuard::requiresRestrictedBrowsing(wholeScriptSpoofUrl)) {
        qCritical() << "RESTRICTED_BROWSING_TEST FAIL: whole-script confusable spoof not flagged";
        return false;
    }

    // Dangerous / restricted ports
    for (int port : {21, 22, 23, 25, 53, 135, 139, 445, 6379, 11211, 27017}) {
        const QUrl restrictedPortUrl(QString("http://example.com:%1/").arg(port));
        if (!NavigationGuard::requiresRestrictedBrowsing(restrictedPortUrl)) {
            qCritical() << "RESTRICTED_BROWSING_TEST FAIL: dangerous port not flagged" << port;
            return false;
        }
        const QString portReason = NavigationGuard::restrictedBrowsingReason(restrictedPortUrl);
        if (portReason.isEmpty() || !portReason.contains("Port réseau")) {
            qCritical() << "RESTRICTED_BROWSING_TEST FAIL: port reason unexpected" << portReason;
            return false;
        }
    }

    // Safe URLs & ports (MUST NOT be flagged as restricted)
    for (const auto &urlStr : {"https://example.com/", "https://cybou.fr/", "https://example.com:443/",
                               "http://example.com:80/", "http://example.com:8080/path",
                               "http://example.com:3000/", "http://example.com:5000/", "http://example.com:8000/", "http://example.com:9000/"}) {
        const QUrl safeUrl(QString::fromLatin1(urlStr));
        if (NavigationGuard::requiresRestrictedBrowsing(safeUrl)) {
            qCritical() << "RESTRICTED_BROWSING_TEST FAIL: safe URL flagged" << urlStr;
            return false;
        }
        if (!NavigationGuard::restrictedBrowsingReason(safeUrl).isEmpty()) {
            qCritical() << "RESTRICTED_BROWSING_TEST FAIL: safe URL has reason" << urlStr;
            return false;
        }
    }

    qInfo() << "NAVIGATION_GUARD_TEST PASS: IPv4/IPv6 private ranges, alternative IPv4 formats, special private domains, DNS rebinding guard, PNA public-to-private blocking, direct/internal allow, IDN homograph checks, restricted ports, restricted browsing mode detection";
    return true;
}
