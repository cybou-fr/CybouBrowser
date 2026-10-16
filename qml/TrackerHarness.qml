import QtQuick
Item {
    required property var browserWindow
    required property var controller
    required property var bar
    property int phase: 0
    onPhaseChanged: console.info("TRACKER_PHASE", phase)
    property string target: ""
    property string oldToken: ""
    property string ownerUid: ""
    property string peerUid: ""
    property int ownerIndex: 0
    property int peerIndex: 0
    property var trackedProfile: null
    property string screenshot: ""
    property var input: null
    property bool oldBlockTrackers: true
    property double observedAt: 0
    property int initialSeconds: 0
    function expect(value, message) { if (!value) throw new Error(message) }
    function begin() {
        oldBlockTrackers = browserStore.blockTrackers
        browserStore.blockTrackers = true
        browserWindow.cleanFeaturePreview()
        target = smokeBaseUrl + "/tracker-document?exact=1"
        browserWindow.navigate(target)
    }
    function step() {
        try {
            const view = browserWindow.currentView
            if (phase === 0 && view.title === "Tracker blocked" && !view.loading) {
                expect(controller.networkSample().some(e=>e.blocked && e.reason === "tracker-rule"), "real tracker request denied before consent")
                expect(!browserStore.canRequestTemporaryTrackers("http://example.test/"), "production HTTP exception not offered")
                expect(controller.setBehavior("storage", 2) && controller.setBehavior("popups", 2), "restrict unrelated settings before exception")
                ownerIndex = browserWindow.activeTab; oldToken = browserWindow.transportState.httpToken
                input = bar.testInputPoint("network", "move"); phase = 1
            } else if (phase === 1 && bar.testCard().visible && bar.selectedData.trackerCanAllow) {
                screenshot = "tracker-before-consent"
                input = bar.testTrackerPoint(false); phase = 2
            } else if (phase === 2 && view.temporaryTrackers && view.title === "Tracker allowed" && !view.loading) {
                expect(view.profile !== browserWindow.activeProfile && view.profile.offTheRecord, "exception profile reserved for this view")
                expect(view.profile.persistentPermissionsPolicy === 0, "no stored positive permissions")
                expect(!view.settings.localStorageEnabled && controller.popupMode(view.url, view.tabUid) === 2, "exception preserves storage and popup restrictions")
                const events = controller.networkSample()
                expect(events.some(e=>!e.blocked && e.decision === "tracker-exception" && e.category === "trackers"), "real tracker reached loopback fixture through exception")
                expect(events.every(e=>!!e.resourceLabel && !e.origin.includes("?")), "human labels and sanitized metadata")
                ownerUid = view.tabUid; trackedProfile = view.profile
                browserWindow.addTab(target); peerIndex = browserWindow.activeTab; phase = 3
            } else if (phase === 3 && view.title === "Tracker blocked" && !view.loading) {
                expect(!view.temporaryContext && view.profile === browserWindow.activeProfile, "peer tab inherits no exception")
                expect(!browserWindow.allowTemporaryTrackers(oldToken) && !browserWindow.revokeTemporaryTrackers(oldToken), "stale token cannot act on peer")
                peerUid = view.tabUid
                browserWindow.activeTab = ownerIndex; browserWindow.currentView.reload(); phase = 4
            } else if (phase === 4 && !view.temporaryContext && view.url.toString() === "about:blank") {
                expect(view.tabUid !== ownerUid, "reload destroys tracker exception")
                browserWindow.navigate(target); phase = 41
            } else if (phase === 41 && view.title === "Tracker blocked" && !view.loading) {
                expect(!browserWindow.allowTemporaryTrackers(oldToken), "old consent cannot repeat")
                expect(browserWindow.allowTemporaryTrackers(browserWindow.transportState.httpToken), "fresh consent after reload")
                input = {kind:"move",x:80,y:300}; phase = 5
            } else if (phase === 5 && view.temporaryTrackers && view.title === "Tracker allowed" && !view.loading) {
                ownerUid = view.tabUid; trackedProfile = view.profile
                input = bar.testInputPoint("network", "move"); phase = 6
            } else if (phase === 6 && bar.testCard().visible) {
                screenshot = "tracker-exception-countdown"; phase = 61
            } else if (phase === 61) {
                browserWindow.activeTab = peerIndex; phase = 7
            } else if (phase === 7) {
                const owner = browserWindow.testViewAt(ownerIndex)
                if (!owner || owner.tabUid === ownerUid) return 0
                expect(!owner.temporaryContext && owner.url.toString() === "about:blank", "60-second expiry destroys exception document")
                expect(browserWindow.activeTab === peerIndex && view.tabUid === peerUid, "background expiry preserves peer selection")
                browserWindow.activeTab = ownerIndex; browserWindow.navigate(target); phase = 8
            } else if (phase === 8 && view.title === "Tracker blocked" && !view.loading) {
                expect(browserWindow.allowTemporaryTrackers(browserWindow.transportState.httpToken), "new visit asks again")
                input = {kind:"move",x:80,y:300}; phase = 9
            } else if (phase === 9 && view.temporaryTrackers && view.title === "Tracker allowed" && !view.loading) {
                trackedProfile = view.profile
                initialSeconds = browserWindow.transportState.trackerSeconds; observedAt = Date.now(); phase = 91
            } else if (phase === 91 && Date.now()-observedAt >= 1500) {
                expect(browserWindow.transportState.trackerSeconds > 0 && browserWindow.transportState.trackerSeconds < initialSeconds, "countdown decreases without extending lease")
                input = bar.testInputPoint("network", "move"); phase = 10
            } else if (phase === 10 && bar.testCard().visible) {
                input = bar.testTrackerPoint(true); phase = 11
            } else if (phase === 11 && !view.temporaryContext && view.url.toString() === "about:blank") {
                browserStore.blockTrackers = oldBlockTrackers
                console.info("TRACKER_TEST PASS: actual blocked/allowed fixture, scoped profile, peer isolation, stale token, reload, countdown, background expiry, panel revoke")
                phase = 12; return 2
            }
            return phase === 12 ? 2 : 0
        } catch (error) {
            browserStore.blockTrackers = oldBlockTrackers
            console.error("TRACKER_TEST FAIL", phase, error); return -1
        }
    }
    function takeScreenshot() { const value = screenshot; screenshot = ""; return value }
    function takeInput() { const value = input; input = null; return value }
}
