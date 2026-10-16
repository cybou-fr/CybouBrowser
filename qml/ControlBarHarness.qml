import QtQuick
Item {
    required property var browserWindow
    required property var coordinator
    required property var controller
    required property var bar
    property int phase: 0
    onPhaseChanged: console.info("OBSERVABLE_PHASE",phase)
    property string screenshot: ""
    property var input: null
    property int sizeIndex: 0
    property var sizes: [[820,560],[1024,768],[1360,900],[1920,1080]]
    property real started: 0
    property real grantObservedAt: 0
    property real firstRemainingMs: 0
    property double lastRequestId: 0
    property string leasedTabUid: ""
    property string rejectedTabUid: ""
    property int resetTabCount: 0
    function expect(value, text) { if (!value) throw new Error(text) }
    function begin() {
        browserWindow.cleanFeaturePreview()
        browserWindow.navigate(smokeBaseUrl + "/controls")
        started = Date.now(); phase = 0
    }
    function step() {
        try {
            const view = browserWindow.currentView
            if (phase === 0 && view.title === "Scripts enabled" && !view.loading) {
                expect(!controller.setPermissionPolicy("camera",1), "camera pregrant denied")
                expect(!controller.setPermissionPolicy("notifications",1), "notification pregrant denied")
                expect(controller.capability("camera").policy === 2, "default unavailable")
                expect(controller.setBehavior("javascript",2), "disable JavaScript")
                expect(view.policyNeedsReload, "reload indication")
                browserWindow.applyViewPolicy(view,view.url); view.reload(); phase = 1
            } else if (phase === 1 && !view.loading && view.title === "Control fixture") {
                expect(!view.settings.javascriptEnabled, "JS-off applied before document")
                controller.setBehavior("javascript",1)
                browserWindow.applyViewPolicy(view,view.url); view.reload(); phase = 2
            } else if (phase === 2 && !view.loading && view.title === "Scripts enabled") {
                expect(controller.networkSample().length > 0, "real network events")
                for (const event of controller.networkSample()) {
                    expect(event.scope === "profile" && event.blocked !== undefined, "explicit network scope and outcome")
                    expect(!event.origin.includes("?") && !event.origin.includes("controls"), "sanitized origins")
                }
                browserWindow.width = sizes[sizeIndex][0]; browserWindow.height = sizes[sizeIndex][1]
                browserWindow.testBrowse(); phase = 3
            } else if (phase === 3) {
                const m = browserWindow.testOmnibox()
                expect(m.visibleKeys.length === 11 && !m.overflowVisible, "all eleven controls at every size")
                expect(m.addressWidth >= 120 && m.menuX > m.downloadX, "address fills available width and controls stay on the right")
                expect(m.separatorX >= m.controlRight && m.downloadX >= m.separatorRight, "separator lies between capability panel and downloads")
                expect(browserWindow.header.height === 74, "capabilities share the navigation row")
                screenshot = "observable-browse-" + sizes[sizeIndex][0]
                input = {kind:"key", key:Qt.Key_L, modifiers:Qt.ControlModifier}; phase = 4
            } else if (phase === 4) {
                const m = browserWindow.testOmnibox()
                console.info("OBSERVABLE_ADDRESS", JSON.stringify(m))
                expect(m.editing && m.selectedText === m.rawUrl, "Ctrl+L selects complete URL")
                expect(m.visibleKeys.length === 11 && m.controlWidth > 0, "URL editing preserves controls")
                screenshot = "observable-edit-" + sizes[sizeIndex][0]
                input = {kind:"key",key:Qt.Key_Escape}; phase = 5
            } else if (phase === 5) {
                expect(!browserWindow.testOmnibox().editing, "Escape collapses address")
                sizeIndex++
                if (sizeIndex < sizes.length) {
                    browserWindow.width = sizes[sizeIndex][0]; browserWindow.height = sizes[sizeIndex][1]; phase = 3
                } else { input = browserWindow.testAddressPoint(); phase = 17 }
            } else if (phase === 17) {
                const m = browserWindow.testOmnibox()
                expect(m.editing && m.addressWidth >= 120 && m.selectedText === m.rawUrl, "mouse selects complete URL")
                expect(m.visibleKeys.length === 11, "mouse edit preserves capabilities")
                input = {kind:"key",key:Qt.Key_Escape}; phase = 18
            } else if (phase === 18) {
                input = bar.testInputPoint("location","move"); phase = 6
            } else if (phase === 6) {
                expect(bar.testCard().visible && bar.testCard().key === "location", "hover opens the current glass control panel")
                const c = bar.testCard()
                expect(c.x >= 0 && c.x+c.width <= browserWindow.width, "glass panel stays inside window")
                screenshot = "controlbar-hover-location"
                input = {kind:"move", x:c.x+40, y:c.y+45}; phase = 24
            } else if (phase === 24) {
                expect(bar.testCard().visible && bar.testCard().pointerInside, "pointer can enter the panel to use its controls")
                input = bar.testInputPoint("javascript","move"); phase = 25
            } else if (phase === 25) {
                expect(bar.testCard().visible && bar.testCard().key === "javascript", "hover replaces content without stale capability")
                screenshot = "controlbar-hover-javascript"
                input = {kind:"move",x:80,y:browserWindow.height-40}; phase = 26
            } else if (phase === 26) {
                if (bar.testCard().visible) return 0
                bar.testFocus(); input = {kind:"key",key:Qt.Key_Right}; phase = 7
            } else if (phase === 7) {
                expect(bar.testFocusedKey() === "notifications", "keyboard arrows")
                view.runJavaScript("Notification.requestPermission().then(p=>document.title='permission-'+p)")
                phase = 8
            } else if (phase === 8 && browserWindow.pendingPermission) {
                expect(controller.capability("notifications").pending, "request badge")
                lastRequestId = coordinator.pendingRequestId
                view.navigationEpoch++
                expect(!coordinator.resolve(true, view, browserWindow.activeProfile, lastRequestId), "same-origin stale epoch cannot grant")
                expect(!view.hasPermissionLease && !browserWindow.pendingPermission, "stale request denied without lease")
                rejectedTabUid = view.tabUid
                resetTabCount = browserWindow.testTabCount()
                input = bar.testInputPoint("notifications","click"); phase = 19
            } else if (phase === 19 && view.title === "Scripts enabled" && !view.loading && view.tabUid !== rejectedTabUid) {
                expect(browserWindow.testTabCount() === resetTabCount, "permission reset replaces the view without adding tabs")
                view.runJavaScript("Notification.requestPermission().then(p=>document.title='permission-'+p)")
                phase = 20
            } else if (phase === 20 && view.title === "permission-denied" && !browserWindow.pendingPermission) {
                throw new Error("native notification denied without a new request in a fresh view")
            } else if (phase === 20 && browserWindow.pendingPermission) {
                expect(coordinator.pendingRequestId > lastRequestId, "new native request has a unique ID")
                expect(!coordinator.resolve(true, view, browserWindow.activeProfile, lastRequestId), "old request ID cannot resolve a new request")
                expect(!!browserWindow.pendingPermission, "rejecting stale ID preserves the live request")
                input = bar.testInputPoint("notifications","click"); phase = 9
            } else if (phase === 9 && view.title === "permission-granted") {
                expect(view.hasPermissionLease, "grant active for document")
                expect(controller.capability("notifications").policy === 1, "temporary policy separate from activity")
                expect(!controller.capability("notifications").pending, "request resolved")
                expect(browserWindow.activeProfile.listAllPermissions().length === 0, "no stored native positive permission")
                leasedTabUid = view.tabUid
                view.runJavaScript("navigator.geolocation.getCurrentPosition(()=>{},()=>{})")
                phase = 21
            } else if (phase === 21 && browserWindow.pendingPermission) {
                expect(controller.capability("location").pending, "second capability has an independent request")
                grantObservedAt = Date.now(); phase = 22
            } else if (phase === 22 && Date.now()-grantObservedAt >= 300) {
                input = bar.testInputPoint("location","click"); phase = 23
            } else if (phase === 23 && controller.capability("location").policy === 1) {
                expect(view.grantedKeys.includes("notifications") && view.grantedKeys.includes("location"), "both native grants tracked")
                input = bar.testInputPoint("javascript","click"); phase = 10
            } else if (phase === 10) {
                expect(view.policyNeedsReload, "direct click changes behavior")
                bar.dismiss()
                coordinator.revokeAll()
                phase = 13
            } else if (phase === 13 && view.url.toString() === "about:blank") {
                expect(!view.hasPermissionLease, "revocation unloads document")
                expect(view.tabUid !== leasedTabUid, "revocation destroys old WebEngineView and replaces its slot")
                expect(!browserWindow.pendingPermission, "revocation leaves no pending request")
                browserWindow.navigate(smokeBaseUrl + "/controls"); phase = 14
            } else if (phase === 14 && view.title === "Scripts enabled" && !view.loading) {
                view.runJavaScript("Notification.requestPermission().then(p=>document.title='timeout-'+p)")
                phase = 15
            } else if (phase === 15 && browserWindow.pendingPermission) {
                expect(controller.capability("notifications").pending, "fresh document requires new consent")
                phase = 16
            } else if (phase === 16 && !browserWindow.pendingPermission && view.title.startsWith("timeout-")) {
                console.info("PERMISSION_TIMEOUT_RESULT", view.title)
                expect(view.title !== "timeout-granted", "timeout never grants")
                expect(!browserWindow.pendingPermission, "unattended request expires")
                expect(controller.capability("notifications").blocked, "timeout recorded as denied")
                browserWindow.addTab(""); phase = 11
            } else if (phase === 11) {
                expect(controller.capability("camera").policy === 3, "blank has no fake site policy")
                expect(browserWindow.testOverlayVisibility(), "overlay contexts dismiss details")
                console.info("CONTROL_BAR_TEST PASS", "elapsedMs",Date.now()-started)
                phase = 12; return 2
            }
            return phase === 12 ? 2 : 0
        } catch(error) { console.error("CONTROL_BAR_TEST FAIL",phase,error); return -1 }
    }
    function takeScreenshot() { const value=screenshot; screenshot=""; return value }
    function takeInput() { const value=input; input=null; return value }
}
