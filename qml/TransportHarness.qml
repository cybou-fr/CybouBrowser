import QtQuick
Item {
    required property var browserWindow
    required property var controller
    required property var bar
    property int phase: 0
    onPhaseChanged: console.info("TRANSPORT_PHASE", phase)
    property string target: ""
    property real chainStarted: 0
    property var chainTargets: []
    property string oldToken: ""
    property string ownerUid: ""
    property string peerUid: ""
    property int ownerIndex: 0
    property int peerIndex: 0
    property var trackedProfile: null
    property string screenshot: ""
    property var input: null
    property double hoverStarted: 0
    property int hoverRetries: 0
    property bool oldHttpsFirst: true
    function expect(value, message) { if (!value) throw new Error(message) }
    function begin() {
        oldHttpsFirst = browserStore.httpsFirst
        browserStore.httpsOnly = true
        browserStore.httpsFirst = false
        browserWindow.cleanFeaturePreview()
        target = smokeBaseUrl + "/transport-document?exact=1"
        browserWindow.navigate(target)
    }
    function step() {
        try {
            const view = browserWindow.currentView
            if (phase >= 13 && Date.now() - chainStarted > 45000) throw new Error("redirect consent chain timeout")
            if (phase === 0 && browserWindow.transportState.httpPending) {
                expect(!view.temporaryTransport && browserStore.httpsOnly, "HTTP denied before consent")
                oldToken = browserWindow.transportState.httpToken
                ownerIndex = browserWindow.activeTab
                input = bar.testInputPoint("network", "move"); hoverStarted = Date.now(); hoverRetries = 0; phase = 1
            } else if (phase === 1 && bar.testCard().visible && bar.selectedData.httpPending) {
                screenshot = "transport-http-blocked"
                input = bar.testHttpAllowPoint(); phase = 2
            } else if (phase === 2 && view.temporaryTransport && view.title === "Transport ready" && !view.loading) {
                expect(view.profile !== browserWindow.activeProfile && view.profile.offTheRecord, "private profile reserved for this HTTP view")
                expect(view.profile.persistentPermissionsPolicy === 0, "HTTP profile never stores positive permissions")
                const sample = controller.networkSample()
                expect(sample.some(e => e.blocked && e.reason === "insecure-transport" && e.origin.includes("localhost")), "cross-origin HTTP subrequest actually blocked")
                expect(sample.some(e => !e.blocked && e.origin.includes("127.0.0.1")), "same-origin HTTP actually allowed")
                expect(sample.some(e => e.blocked && e.reason === "insecure-transport" && e.origin.startsWith("ws:")), "temporary HTTP consent does not grant WebSocket access")
                expect(sample.every(e => !e.origin.includes("?") && !e.origin.includes("exact=")), "network sample retains no query")
                ownerUid = view.tabUid
                trackedProfile = view.profile
                browserWindow.addTab(target); peerIndex = browserWindow.activeTab; phase = 3
            } else if (phase === 3 && browserWindow.transportState.httpPending) {
                expect(!view.temporaryTransport && view.profile === browserWindow.activeProfile, "other tab inherits no HTTP allowance")
                expect(!browserWindow.allowTemporaryHttp(oldToken), "old consent token cannot act on another tab")
                peerUid = view.tabUid
                browserWindow.activeTab = ownerIndex
                browserWindow.currentView.reload(); phase = 4
            } else if (phase === 4 && browserWindow.transportState.httpPending && !view.temporaryTransport) {
                expect(view.tabUid !== ownerUid, "reload destroys the exception context")
                expect(!browserWindow.allowTemporaryHttp(oldToken), "old token cannot repeat navigation")
                expect(browserWindow.allowTemporaryHttp(browserWindow.transportState.httpToken), "fresh explicit consent")
                input = {x:80, y:300, kind:"move"}
                phase = 5
            } else if (phase === 5 && view.temporaryTransport && view.title === "Transport ready" && !view.loading) {
                ownerUid = view.tabUid
                trackedProfile = view.profile
                input = bar.testInputPoint("network", "move"); hoverStarted = Date.now(); hoverRetries = 0; phase = 6
            } else if (phase === 6 && bar.testCard().visible) {
                screenshot = "transport-http-temporary"
                input = bar.testHttpRevokePoint(); phase = 7
            } else if (phase === 7 && view.url.toString() === "about:blank" && !view.temporaryTransport) {
                expect(view.tabUid !== ownerUid, "popover revoke destroys the HTTP view")
                const peer = browserWindow.testViewAt(peerIndex)
                expect(peer && peer.tabUid === peerUid && !peer.temporaryTransport, "revoking one tab preserves the other tab")
                browserWindow.navigate(target); phase = 8
            } else if (phase === 8 && browserWindow.transportState.httpPending) {
                expect(!view.temporaryTransport, "later visit is denied again")
                input = browserWindow.testHttpButtonPoint()
                phase = 9
            } else if (phase === 9 && view.temporaryTransport && view.title === "Transport ready" && !view.loading) {
                trackedProfile = view.profile
                input = bar.testInputPoint("network", "move"); phase = 10
            } else if (phase === 10 && bar.testCard().visible) {
                input = browserWindow.testHttpButtonPoint(); phase = 11
            } else if (phase === 11 && view.url.toString() === "about:blank" && !view.temporaryTransport) {
                chainStarted = Date.now()
                chainTargets = [smokeBaseUrl + "/ui-redirect?hop=1", transportRedirectPeerBase + "/ui-redirect?hop=2", smokeBaseUrl + "/ui-redirect?hop=3"]
                browserWindow.navigate(chainTargets[0]); phase = 13
            } else if (phase === 13 && browserWindow.transportState.httpPending) {
                expect(view.blockedHttpUrl === chainTargets[0], "first redirect URL awaits consent")
                oldToken = browserWindow.transportState.httpToken; ownerUid = view.tabUid
                input = browserWindow.testHttpButtonPoint(); phase = 14
            } else if (phase === 14 && browserWindow.transportState.httpPending && !view.temporaryTransport && view.blockedHttpUrl === chainTargets[1]) {
                expect(view.tabUid !== ownerUid && browserWindow.transportState.httpToken !== oldToken, "cross-origin redirect has a new context and token")
                expect(!browserWindow.allowTemporaryHttp(oldToken), "first-hop token cannot authorize the second hop")
                oldToken = browserWindow.transportState.httpToken; ownerUid = view.tabUid
                input = browserWindow.testHttpButtonPoint(); phase = 15
            } else if (phase === 15 && browserWindow.transportState.httpPending && !view.temporaryTransport && view.blockedHttpUrl === chainTargets[2]) {
                expect(view.tabUid !== ownerUid && browserWindow.transportState.httpToken !== oldToken, "307 redirect requires a third context and token")
                expect(!browserWindow.allowTemporaryHttp(oldToken), "second-hop token cannot authorize the terminal URL")
                oldToken = browserWindow.transportState.httpToken
                input = browserWindow.testHttpButtonPoint(); phase = 16
            } else if (phase === 16 && view.temporaryTransport && view.title === "Redirect terminal" && !view.loading) {
                expect(view.profile.offTheRecord && view.profile !== browserWindow.activeProfile, "terminal URL still uses an isolated profile")
                trackedProfile = view.profile; ownerUid = view.tabUid
                input = browserWindow.testHttpButtonPoint(); phase = 17
            } else if (phase === 17 && view.url.toString() === "about:blank" && !view.temporaryTransport) {
                expect(view.tabUid !== ownerUid && !browserWindow.allowTemporaryHttp(oldToken), "terminal revoke invalidates the redirect consent")
                const peer = browserWindow.testViewAt(peerIndex)
                expect(peer && peer.tabUid === peerUid && !peer.temporaryTransport, "redirect chain leaves the peer tab untouched")
                console.info("HTTP_REDIRECT_UI_CHAIN PASS: 302 and 307 cross-origin hops require separate UI consent and reject stale tokens")
                browserStore.httpsFirst = oldHttpsFirst
                console.info("TRANSPORT_TEST PASS: exact navigation, same-origin resource gate, other tab, reload, stale token, popover revoke, inline revoke")
                phase = 12; return 2
            } else if ((phase === 1 || phase === 6) && !bar.testCard().visible && Date.now() - hoverStarted > 2000) {
                // One synthetic move can be lost; leave and re-enter the icon until the card opens.
                input = (++hoverRetries % 2) ? {x:80, y:300, kind:"move"} : bar.testInputPoint("network", "move")
                if (hoverRetries === 1) console.warn("TRANSPORT_HOVER_RETRY phase", phase)
            }
            return phase === 12 ? 2 : 0
        } catch(error) {
            browserStore.httpsFirst = oldHttpsFirst
            console.error("TRANSPORT_TEST FAIL", phase, error); return -1
        }
    }
    function takeScreenshot() { const value = screenshot; screenshot = ""; return value }
    function takeInput() { const value = input; input = null; return value }
}
