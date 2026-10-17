#pragma once
#include <QUrl>
#include <QString>
#include <QStringList>

#include <array>

namespace TrackerBlocker {

using namespace Qt::StringLiterals;

inline bool isSameOrSubdomain(QStringView host, QStringView base) {
    if (base.isEmpty() || host.isEmpty()) return false;
    if (host == base) return true;
    return host.endsWith(base) && host.size() > base.size() && host[host.size() - base.size() - 1] == u'.';
}

inline bool isTrackerHost(QStringView host) {
    static constexpr std::array<QStringView, 64> trackerSuffixes = {
        u"doubleclick.net",
        u"google-analytics.com",
        u"googletagmanager.com",
        u"googlesyndication.com",
        u"adservice.google.com",
        u"pagead2.googlesyndication.com",
        u"adnxs.com",
        u"appnexus.com",
        u"criteo.com",
        u"criteo.net",
        u"outbrain.com",
        u"taboola.com",
        u"rubiconproject.com",
        u"pubmatic.com",
        u"openx.net",
        u"casalemedia.com",
        u"scorecardresearch.com",
        u"hotjar.com",
        u"clarity.ms",
        u"segment.io",
        u"segment.com",
        u"mixpanel.com",
        u"amplitude.com",
        u"heapanalytics.com",
        u"mouseflow.com",
        u"fullstory.com",
        u"connect.facebook.net",
        u"pixel.facebook.com",
        u"an.facebook.com",
        u"bat.bing.com",
        u"aax.amazon-adsystem.com",
        u"amazon-adsystem.com",
        u"mc.yandex.ru",
        u"advertising.com",
        u"adroll.com",
        u"quantserve.com",
        u"chartbeat.com",
        u"moatads.com",
        u"smartadserver.com",
        u"indexexchange.com",
        u"sovrn.com",
        u"triplelift.com",
        u"sharethrough.com",
        u"media.net",
        u"bidswitch.net",
        u"smartclip.net",
        u"teads.tv",
        u"mgid.com",
        u"revcontent.com",
        u"branch.io",
        u"appsflyer.com",
        u"adjust.com",
        u"braze.com",
        u"onesignal.com",
        u"analytics.twitter.com",
        u"static.ads-twitter.com",
        u"ads.linkedin.com",
        u"snap.licdn.com",
        u"sc-static.net",
        u"tr.snapchat.com",
        u"analytics.tiktok.com",
        u"ads.pinterest.com",
        u"ct.pinterest.com",
        u"pixel.quora.com"
    };
    for (const auto &suffix : trackerSuffixes) {
        if (isSameOrSubdomain(host, suffix))
            return true;
    }
    return false;
}

inline bool isTracker(const QUrl &requestUrl, const QUrl &firstPartyUrl) {
    const QString host = requestUrl.host().toLower();
    if (host.isEmpty()) return false;
    
    // Allow same-origin / first-party requests
    if (firstPartyUrl.isValid()) {
        const QString firstPartyHost = firstPartyUrl.host().toLower();
        if (isSameOrSubdomain(host, firstPartyHost) || isSameOrSubdomain(firstPartyHost, host))
            return false;
    }
        
    return isTrackerHost(host);
}

} // namespace TrackerBlocker
