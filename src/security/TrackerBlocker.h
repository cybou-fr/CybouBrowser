#pragma once
#include "RuleEngine.h"

namespace TrackerBlocker {

inline bool isSameOrSubdomain(QStringView host, QStringView base) {
    return RuleEngine::isSameOrSubdomain(host, base);
}

inline bool isTrackerHost(QStringView host) {
    for (const auto &rule : RuleEngine::rules) {
        if (RuleEngine::isSameOrSubdomain(host, rule.suffix))
            return true;
    }
    return false;
}

inline bool isTracker(const QUrl &requestUrl, const QUrl &firstPartyUrl) {
    return RuleEngine::classify(requestUrl, firstPartyUrl).matched;
}

} // namespace TrackerBlocker
