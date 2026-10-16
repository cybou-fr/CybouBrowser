#pragma once
#include <QUrl>
#include <QString>
#include <QStringList>

namespace TrackerBlocker {

using namespace Qt::StringLiterals;

inline bool isTrackerHost(QStringView host) {
    static const QStringList trackerSuffixes = {
        u"doubleclick.net"_s,
        u"google-analytics.com"_s,
        u"googletagmanager.com"_s,
        u"googlesyndication.com"_s,
        u"adservice.google.com"_s,
        u"pagead2.googlesyndication.com"_s,
        u"adnxs.com"_s,
        u"appnexus.com"_s,
        u"criteo.com"_s,
        u"criteo.net"_s,
        u"outbrain.com"_s,
        u"taboola.com"_s,
        u"rubiconproject.com"_s,
        u"pubmatic.com"_s,
        u"openx.net"_s,
        u"casalemedia.com"_s,
        u"scorecardresearch.com"_s,
        u"hotjar.com"_s,
        u"clarity.ms"_s,
        u"segment.io"_s,
        u"segment.com"_s,
        u"mixpanel.com"_s,
        u"connect.facebook.net"_s,
        u"pixel.facebook.com"_s,
        u"an.facebook.com"_s,
        u"bat.bing.com"_s,
        u"aax.amazon-adsystem.com"_s,
        u"amazon-adsystem.com"_s,
        u"mc.yandex.ru"_s,
        u"advertising.com"_s,
        u"adroll.com"_s,
        u"quantserve.com"_s,
        u"chartbeat.com"_s,
        u"moatads.com"_s,
        u"smartadserver.com"_s
    };
    for (const auto &suffix : trackerSuffixes) {
        if (host == suffix || host.endsWith(u"." + suffix))
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
        if (host == firstPartyHost || (firstPartyHost.length() > 0 && host.endsWith(u"." + firstPartyHost)))
            return false;
    }
        
    return isTrackerHost(host);
}

} // namespace TrackerBlocker
