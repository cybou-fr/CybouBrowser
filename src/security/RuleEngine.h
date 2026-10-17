#pragma once
#include <QUrl>
#include <QString>
#include <QStringView>
#include <array>
#include <cstdint>

namespace RuleEngine {

using namespace Qt::StringLiterals;

enum class RuleCategory : uint8_t {
    None = 0,
    Tracker = 1,
    Ad = 2
};

struct MatchResult {
    bool matched = false;
    RuleCategory category = RuleCategory::None;
    QStringView rule;
    QStringView categoryName() const {
        switch (category) {
        case RuleCategory::Tracker: return u"tracker";
        case RuleCategory::Ad: return u"ad";
        default: return u"none";
        }
    }
};

inline constexpr int version() { return 1; }
inline constexpr uint32_t integrityChecksum() { return 0xA4D17C32; }

inline bool isSameOrSubdomain(QStringView host, QStringView base) {
    if (base.isEmpty() || host.isEmpty()) return false;
    if (host == base) return true;
    return host.endsWith(base) && host.size() > base.size() && host[host.size() - base.size() - 1] == u'.';
}

// Protected infrastructure and false-positive immunity list:
// Critical CDNs, payment processors, SSO authentication and CAPTCHAs must never be blocked.
inline bool isProtectedHost(QStringView host) {
    static constexpr std::array<QStringView, 14> protectedSuffixes = {
        u"cdnjs.cloudflare.com",
        u"cdn.jsdelivr.net",
        u"unpkg.com",
        u"js.stripe.com",
        u"m.stripe.network",
        u"checkout.stripe.com",
        u"paypal.com",
        u"pay.google.com",
        u"accounts.google.com",
        u"login.microsoftonline.com",
        u"appleid.apple.com",
        u"recaptcha.net",
        u"hcaptcha.com",
        u"challenges.cloudflare.com"
    };
    for (const auto &suffix : protectedSuffixes) {
        if (isSameOrSubdomain(host, suffix)) return true;
    }
    return false;
}

struct RuleEntry {
    QStringView suffix;
    RuleCategory category;
};

// Versioned, categorized rule list:
inline constexpr std::array<RuleEntry, 64> rules = {{
    // Ads
    { u"doubleclick.net", RuleCategory::Ad },
    { u"googlesyndication.com", RuleCategory::Ad },
    { u"adservice.google.com", RuleCategory::Ad },
    { u"pagead2.googlesyndication.com", RuleCategory::Ad },
    { u"adnxs.com", RuleCategory::Ad },
    { u"appnexus.com", RuleCategory::Ad },
    { u"criteo.com", RuleCategory::Ad },
    { u"criteo.net", RuleCategory::Ad },
    { u"outbrain.com", RuleCategory::Ad },
    { u"taboola.com", RuleCategory::Ad },
    { u"rubiconproject.com", RuleCategory::Ad },
    { u"pubmatic.com", RuleCategory::Ad },
    { u"openx.net", RuleCategory::Ad },
    { u"casalemedia.com", RuleCategory::Ad },
    { u"aax.amazon-adsystem.com", RuleCategory::Ad },
    { u"amazon-adsystem.com", RuleCategory::Ad },
    { u"advertising.com", RuleCategory::Ad },
    { u"adroll.com", RuleCategory::Ad },
    { u"smartadserver.com", RuleCategory::Ad },
    { u"indexexchange.com", RuleCategory::Ad },
    { u"sovrn.com", RuleCategory::Ad },
    { u"triplelift.com", RuleCategory::Ad },
    { u"sharethrough.com", RuleCategory::Ad },
    { u"media.net", RuleCategory::Ad },
    { u"bidswitch.net", RuleCategory::Ad },
    { u"smartclip.net", RuleCategory::Ad },
    { u"teads.tv", RuleCategory::Ad },
    { u"mgid.com", RuleCategory::Ad },
    { u"revcontent.com", RuleCategory::Ad },
    { u"moatads.com", RuleCategory::Ad },
    { u"static.ads-twitter.com", RuleCategory::Ad },
    { u"ads.linkedin.com", RuleCategory::Ad },
    { u"ads.pinterest.com", RuleCategory::Ad },
    { u"an.facebook.com", RuleCategory::Ad },

    // Trackers
    { u"google-analytics.com", RuleCategory::Tracker },
    { u"googletagmanager.com", RuleCategory::Tracker },
    { u"scorecardresearch.com", RuleCategory::Tracker },
    { u"hotjar.com", RuleCategory::Tracker },
    { u"clarity.ms", RuleCategory::Tracker },
    { u"segment.io", RuleCategory::Tracker },
    { u"segment.com", RuleCategory::Tracker },
    { u"mixpanel.com", RuleCategory::Tracker },
    { u"amplitude.com", RuleCategory::Tracker },
    { u"heapanalytics.com", RuleCategory::Tracker },
    { u"mouseflow.com", RuleCategory::Tracker },
    { u"fullstory.com", RuleCategory::Tracker },
    { u"connect.facebook.net", RuleCategory::Tracker },
    { u"pixel.facebook.com", RuleCategory::Tracker },
    { u"bat.bing.com", RuleCategory::Tracker },
    { u"mc.yandex.ru", RuleCategory::Tracker },
    { u"quantserve.com", RuleCategory::Tracker },
    { u"chartbeat.com", RuleCategory::Tracker },
    { u"branch.io", RuleCategory::Tracker },
    { u"appsflyer.com", RuleCategory::Tracker },
    { u"adjust.com", RuleCategory::Tracker },
    { u"braze.com", RuleCategory::Tracker },
    { u"onesignal.com", RuleCategory::Tracker },
    { u"analytics.twitter.com", RuleCategory::Tracker },
    { u"snap.licdn.com", RuleCategory::Tracker },
    { u"sc-static.net", RuleCategory::Tracker },
    { u"tr.snapchat.com", RuleCategory::Tracker },
    { u"analytics.tiktok.com", RuleCategory::Tracker },
    { u"ct.pinterest.com", RuleCategory::Tracker },
    { u"pixel.quora.com", RuleCategory::Tracker }
}};

inline constexpr size_t ruleCount() { return rules.size(); }

inline MatchResult classify(const QUrl &requestUrl, const QUrl &firstPartyUrl) {
    const QString hostStr = requestUrl.host().toLower();
    if (hostStr.isEmpty()) return {};
    const QStringView host(hostStr);

    // 1. False-positive immunity check
    if (isProtectedHost(host)) return {};

    // 2. Allow same-origin / first-party requests
    if (firstPartyUrl.isValid()) {
        const QString firstPartyHostStr = firstPartyUrl.host().toLower();
        if (!firstPartyHostStr.isEmpty()) {
            const QStringView firstPartyHost(firstPartyHostStr);
            if (isSameOrSubdomain(host, firstPartyHost) || isSameOrSubdomain(firstPartyHost, host))
                return {};
        }
    }

    // 3. Scan versioned rule categories
    for (const auto &rule : rules) {
        if (isSameOrSubdomain(host, rule.suffix)) {
            return { true, rule.category, rule.suffix };
        }
    }

    return {};
}

} // namespace RuleEngine
