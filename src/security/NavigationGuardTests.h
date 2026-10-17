#pragma once
#include "NavigationGuard.h"
#include <QDebug>

inline bool runNavigationGuardTests() {
    // 1. Loopback addresses
    for (const auto &urlStr : {"http://127.0.0.1/", "http://127.0.0.5:8080/", "http://127.255.255.255/",
                               "http://localhost/", "http://test.localhost/", "http://[::1]/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isLocalHost(url) || !NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: loopback classification" << urlStr;
            return false;
        }
    }

    // 2. Private IPv4 addresses (RFC 1918, RFC 3927 link-local, RFC 6598 CGNAT, RFC 1122)
    for (const auto &urlStr : {"http://10.1.2.3/", "http://172.16.5.6/", "http://172.31.255.254/",
                               "http://192.168.1.1/", "http://169.254.169.254/", "http://169.254.10.20/",
                               "http://100.64.0.1/", "http://0.0.0.0/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: private IPv4 classification" << urlStr;
            return false;
        }
    }

    // 3. Private IPv6 addresses (ULA fc00::/7, Link-Local fe80::/10, IPv4-mapped IPv6)
    for (const auto &urlStr : {"http://[fc00::1]/", "http://[fd12:3456:789a::1]/", "http://[fe80::1]/",
                               "http://[::ffff:192.168.1.1]/", "http://[::ffff:127.0.0.1]/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (!NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: private IPv6 classification" << urlStr;
            return false;
        }
    }

    // 4. Public addresses (MUST NOT be classified as private network)
    for (const auto &urlStr : {"https://8.8.8.8/", "https://1.1.1.1/", "https://142.250.190.46/",
                               "https://172.15.255.255/", "https://172.32.0.1/",
                               "https://example.com/", "https://cybou.fr/"}) {
        const QUrl url(QString::fromLatin1(urlStr));
        if (NavigationGuard::isPrivateNetwork(url)) {
            qCritical() << "NAVIGATION_GUARD_TEST FAIL: public classified as private" << urlStr;
            return false;
        }
    }

    // 5. Cross-network Private Network Access (PNA) protection
    const QUrl publicFirstParty("https://attacker.example/page");
    const QUrl internalFirstParty("http://192.168.1.10:8080/dashboard");
    const QUrl localFirstParty("http://localhost:3000/app");

    // Public -> Private subresources: MUST be blocked
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/admin"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 192.168.1.1 subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://127.0.0.1:8080/api"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 127.0.0.1 subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://localhost:9000/"), publicFirstParty, false)) { qCritical() << "PNA FAIL: localhost subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://169.254.169.254/latest/meta-data/"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 169.254 subresource"; return false; }
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://10.0.0.5/secret"), publicFirstParty, false)) { qCritical() << "PNA FAIL: 10.0.0.5 subresource"; return false; }

    // Public -> Private mainFrame navigation attempt: MUST be blocked
    if (!NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/reboot"), publicFirstParty, true)) { qCritical() << "PNA FAIL: 192.168.1.1 mainFrame"; return false; }

    // Internal -> Internal: MUST be allowed
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/api"), internalFirstParty, false)) { qCritical() << "PNA FAIL: internal->internal"; return false; }
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://localhost:3000/api"), localFirstParty, false)) { qCritical() << "PNA FAIL: local->local"; return false; }

    // Direct address bar navigation (empty first-party): MUST be allowed
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://192.168.1.1/"), QUrl(), true)) { qCritical() << "PNA FAIL: direct 192.168.1.1"; return false; }
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("http://localhost:8080/"), QUrl(), true)) { qCritical() << "PNA FAIL: direct localhost"; return false; }

    // Public -> Public: MUST be allowed
    if (NavigationGuard::blocksPrivateNetworkAccess(QUrl("https://cdn.example/lib.js"), publicFirstParty, false)) { qCritical() << "PNA FAIL: public->public"; return false; }

    // 6. Suspicious IDN / Homograph detection
    if (NavigationGuard::hasSuspiciousIdn(QUrl("https://example.com/"))) { qCritical() << "IDN FAIL: example.com"; return false; }
    if (NavigationGuard::hasSuspiciousIdn(QUrl("https://xn--bcher-kva.example/"))) { qCritical() << "IDN FAIL: bücher.example"; return false; }
    if (NavigationGuard::hasSuspiciousIdn(QUrl("https://xn--e1afmkfd.xn--p1ai/"))) { qCritical() << "IDN FAIL: пример.рф"; return false; }
    if (!NavigationGuard::hasSuspiciousIdn(QUrl("https://xn--pple-43d.example/"))) { qCritical() << "IDN FAIL: аpple.example"; return false; }

    qInfo() << "NAVIGATION_GUARD_TEST PASS: IPv4/IPv6 private ranges, PNA public-to-private blocking, direct/internal allow, IDN homograph checks";
    return true;
}
