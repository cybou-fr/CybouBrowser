#pragma once
#include "ActivityAggregator.h"
#include <QDebug>

inline bool runActivityAggregatorTests() {
    ActivityAggregator aggregator(10);

    // 1. Record permission requests and verify tab scope
    aggregator.recordPermission("camera", "tab-1", "https://top.example", "https://top.example", true, false, 101);
    aggregator.recordPermission("mic", "tab-1", "https://top.example", "https://frame.example", true, false, 102);
    aggregator.recordPermission("camera", "tab-2", "https://other.example", "https://other.example", true, false, 201);

    if (aggregator.count() != 3) return false;

    auto summaryCamera1 = aggregator.summarize("camera", "tab-1", "https://top.example");
    if (!summaryCamera1.pending || summaryCamera1.blocked || summaryCamera1.events.size() != 1) return false;

    auto eventCamera1 = summaryCamera1.events.first().toMap();
    if (eventCamera1.value("scope").toString() != "tab") return false;
    if (eventCamera1.value("key").toString() != "camera") return false;
    if (eventCamera1.value("origin").toString() != "https://top.example") return false;

    auto summaryMic1 = aggregator.summarize("mic", "tab-1", "https://top.example");
    if (!summaryMic1.pending || summaryMic1.requester != "https://frame.example") return false;

    // 2. Finish permission: resolve one as allowed, one as blocked
    aggregator.finishPermission("camera", "tab-1", "https://top.example", true, 101);
    aggregator.finishPermission("mic", "tab-1", "https://frame.example", false, 102);

    summaryCamera1 = aggregator.summarize("camera", "tab-1", "https://top.example");
    if (summaryCamera1.pending || summaryCamera1.blocked || !summaryCamera1.recent) return false;

    summaryMic1 = aggregator.summarize("mic", "tab-1", "https://top.example");
    if (summaryMic1.pending || !summaryMic1.blocked || !summaryMic1.recent) return false;

    // 3. Record popup attempt
    aggregator.recordPopup("tab-1", "https://top.example", true);
    auto summaryPopups = aggregator.summarize("popups", "tab-1", "https://top.example");
    if (!summaryPopups.blocked || summaryPopups.events.size() != 1) return false;
    if (summaryPopups.events.first().toMap().value("scope").toString() != "tab") return false;

    // 4. Tab context expiration: tab-1 expired, tab-2 preserved
    aggregator.expireTab("tab-1");
    if (!aggregator.summarize("camera", "tab-1", "https://top.example").events.isEmpty()) return false;
    if (!aggregator.summarize("mic", "tab-1", "https://top.example").events.isEmpty()) return false;
    if (aggregator.summarize("camera", "tab-2", "https://other.example").events.isEmpty()) return false;

    // 5. Bounded capacity test
    aggregator.clear();
    for (int i = 0; i < 20; ++i) {
        aggregator.recordPermission("camera", QString("tab-%1").arg(i), "https://example.com", "https://example.com", false, false, i);
    }
    if (aggregator.count() > 10) return false;

    qInfo() << "ACTIVITY_AGGREGATOR_TEST PASS: truthful tab/profile scopes, bounded queue, status separation, tab context expiration";
    return true;
}
