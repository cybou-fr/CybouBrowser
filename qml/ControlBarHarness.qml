import QtQuick
Item {
    required property var browserWindow
    required property var controller
    required property var bar
    property int phase: 0
    onPhaseChanged: console.info("OBSERVABLE_PHASE",phase)
    property string screenshot: ""
    property var input: null
    property int sizeIndex: 0
    property var sizes: [[820,560],[1024,768],[1360,900],[1920,1080]]
    property real started: 0
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
                expect(m.addressWidth >= 160 && m.menuX > m.downloadX, "address fills available width and controls stay on the right")
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
                expect(m.editing && m.addressWidth >= 160 && m.selectedText === m.rawUrl, "mouse selects complete URL")
                expect(m.visibleKeys.length === 11, "mouse edit preserves capabilities")
                input = {kind:"key",key:Qt.Key_Escape}; phase = 18
            } else if (phase === 18) {
                input = bar.testInputPoint("location","move"); phase = 6
            } else if (phase === 6) {
                expect(!bar.testCard().visible, "hover never opens shared details")
                bar.testFocus(); input = {kind:"key",key:Qt.Key_Right}; phase = 7
            } else if (phase === 7) {
                expect(bar.testFocusedKey() === "notifications", "keyboard arrows")
                view.runJavaScript("Notification.requestPermission().then(p=>document.title='permission-'+p)")
                phase = 8
            } else if (phase === 8 && browserWindow.pendingPermission) {
                expect(controller.capability("notifications").pending, "request badge")
                input = bar.testInputPoint("notifications","click"); phase = 9
            } else if (phase === 9 && view.title === "permission-granted") {
                expect(view.permissionLease.running, "grant has deadline")
                expect(controller.capability("notifications").policy === 1, "temporary policy separate from activity")
                expect(!controller.capability("notifications").pending, "request resolved")
                expect(browserWindow.activeProfile.listAllPermissions().length === 0, "no stored native positive permission")
                input = bar.testInputPoint("javascript","click"); phase = 10
            } else if (phase === 10) {
                expect(view.policyNeedsReload, "direct click changes behavior")
                expect(!bar.testCard().visible, "direct click has no permission popover")
                phase = 13
            } else if (phase === 13 && view.url.toString() === "about:blank") {
                expect(!view.permissionLease.running, "expired grant unloads document")
                expect(!browserWindow.pendingPermission, "expiry leaves no pending request")
                browserWindow.navigate(smokeBaseUrl + "/controls"); phase = 14
            } else if (phase === 14 && view.title === "Scripts enabled" && !view.loading) {
                view.runJavaScript("Notification.requestPermission().then(p=>document.title='timeout-'+p)")
                phase = 15
            } else if (phase === 15 && browserWindow.pendingPermission) {
                expect(controller.capability("notifications").pending, "fresh document requires new consent")
                phase = 16
            } else if (phase === 16 && view.title === "timeout-denied") {
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
