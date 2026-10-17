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
            } else if (phase === 2 && browserWindow.transportState.trackerTemporary && view.title === "Tracker allowed" && !view.loading) {
                expect(view.profile === browserWindow.activeProfile, "exception applies on current profile")
                expect(view.profile.persistentPermissionsPolicy === 0, "no stored positive permissions")
                expect(!view.settings.localStorageEnabled && controller.popupMode(view.url, view.tabUid) === 2, "exception preserves storage and popup restrictions")
                const events = controller.networkSample()
                expect(events.some(e=>!e.blocked && e.decision === "tracker-exception" && e.category === "trackers"), "real tracker reached loopback fixture through exception")
                expect(events.every(e=>!!e.resourceLabel && !e.origin.includes("?")), "human labels and sanitized metadata")
                ownerUid = view.tabUid; trackedProfile = view.profile
                browserWindow.addTab(target); peerIndex = browserWindow.activeTab; phase = 3
            } else if (phase === 3 && view.title === "Tracker allowed" && !view.loading) {
                // Peer tab with the same origin shares the active exception (ADR 0005 truthful UI)
                expect(browserWindow.transportState.trackerTemporary, "peer tab on same origin truthfully sees active exception")
                peerUid = view.tabUid
                // Closing owner tab revokes the exception
                browserWindow.closeTab(ownerIndex); phase = 4
            } else if (phase === 4) {
                // Owner closed, peer tab reloaded will now have trackers blocked
                browserWindow.currentView.reload(); phase = 41
            } else if (phase === 41 && view.title === "Tracker blocked" && !view.loading) {
                expect(!browserWindow.transportState.trackerTemporary, "exception revoked after owner left origin")
                expect(browserWindow.allowTemporaryTrackers(browserWindow.transportState.httpToken), "fresh consent granted")
                input = {kind:"move",x:80,y:300}; phase = 5
            } else if (phase === 5 && browserWindow.transportState.trackerTemporary && view.title === "Tracker allowed" && !view.loading) {
                input = bar.testInputPoint("network", "move"); phase = 6
            } else if (phase === 6 && bar.testCard().visible) {
                screenshot = "tracker-exception-active"
                input = bar.testTrackerPoint(true); phase = 7
            } else if (phase === 7 && view.title === "Tracker blocked" && !view.loading) {
                expect(!browserWindow.transportState.trackerTemporary, "panel revoke cleared exception and reloaded")
                console.info("TRACKER_TEST PASS: actual blocked/allowed fixture, origin scope, truthful peer visibility, owner departure revoke, panel revoke")
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
