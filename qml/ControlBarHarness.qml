import QtQuick
import QtWebEngine

QtObject {
    required property var browserWindow
    required property var controller
    required property var bar
    property int phase: 0
    property string screenshot: ""
    property var input: null
    property string siteA: ""
    property string siteB: ""
    property int beforeTabs: 0
    property real started: 0
    property string oldLanguage: "fr"
    property real wideAddress: 0
    property real narrowAddress: 0
    property var frozenKeys: []
    property string chosenKey: ""
    function begin() {
        oldLanguage = browserStore.controlBarLanguage
        browserWindow.cleanFeaturePreview()
        siteA = smokeBaseUrl + "/controls"
        siteB = smokeBaseUrl.replace("127.0.0.1", "localhost") + "/controls"
        controller.setBehavior("javascript", 1)
        browserWindow.navigate(siteA)
        started = Date.now(); phase = 0
    }
    function expect(value, message) { if (!value) throw new Error(message) }
    function step() {
        try {
            const view = browserWindow.currentView
            if (phase === 0 && view.title === "Scripts enabled" && !view.loading) {
                expect(controller.setBehavior("javascript", 2), "set JS origin policy")
                expect(view.policyNeedsReload, "reload marker")
                expect(controller.capability("javascript").policy === 4, "truthful reload state")
                browserWindow.applyViewPolicy(view, view.url); view.reload(); phase = 1
            } else if (phase === 1 && view.title === "Control fixture" && !view.loading) {
                expect(!view.settings.javascriptEnabled, "JS disabled after reload")
                browserWindow.addTab(siteB); phase = 2
            } else if (phase === 2 && view.title === "Scripts enabled" && !view.loading) {
                expect(view.settings.javascriptEnabled, "JS isolation between origins")
                expect(!controller.settingFor(siteA, "javascript"), "origin policy retained")
                expect(controller.setBehavior("storage", 2), "localStorage policy")
                browserWindow.applyViewPolicy(view, view.url); view.reload(); phase = 3
            } else if (phase === 3 && view.title === "Storage blocked" && !view.loading) {
                expect(controller.setBehavior("storage", 1), "restore localStorage")
                browserWindow.applyViewPolicy(view, view.url); view.reload(); phase = 4
            } else if (phase === 4 && view.title === "Scripts enabled" && !view.loading) {
                expect(!controller.setPermissionPolicy("camera", 1), "no arbitrary ephemeral pregrant")
                expect(controller.setPermissionPolicy("camera", 2) && controller.capability("camera").policy === 2, "camera deny policy")
                controller.clearPermissionPolicies()
                expect(controller.capability("camera").policy === 0, "custom permission cleanup")
                expect(controller.setPermissionPolicy("notifications", 2), "persistent deny")
                expect(controller.capability("notifications").policy === 2, "native deny readback")
                expect(controller.setPermissionPolicy("notifications", 1), "persistent allow")
                expect(controller.capability("notifications").policy === 1, "native allow readback")
                expect(controller.setPermissionPolicy("notifications", 0), "persistent reset")
                expect(controller.capability("screen").policy === 3, "screen unavailable")
                controller.setBehavior("popups", 0)
                beforeTabs = browserWindow.testTabCount()
                view.runJavaScript("window.open('/popup-target','_blank')")
                phase = 5
            } else if (phase === 5 && controller.capability("popups").blocked) {
                expect(browserWindow.testTabCount() === beforeTabs, "unsolicited popup blocked")
                controller.setBehavior("popups", 1)
                view.runJavaScript("window.open('/popup-target','_blank')")
                phase = 6
            } else if (phase === 6 && browserWindow.testTabCount() === beforeTabs + 1 && !view.loading) {
                controller.setBehavior("popups", 0)
                expect(controller.networkSample().length > 0, "real network sample")
                for (const event of controller.networkSample()) expect(!event.origin.includes("?") && !event.origin.includes("@") && !event.origin.includes("controls"), "sanitized origin metadata")
                view.runJavaScript("Promise.all(Array.from({length:80},(_,i)=>fetch('/asset?secret=not-retained&i='+i))).then(()=>document.title='Burst done')")
                phase = 7
            } else if (phase === 7 && view.title === "Burst done") {
                expect(controller.networkSample().length <= 256 && controller.networkSample().length >= 80, "bounded network sample")
                controller.setPermissionPolicy("notifications", 0)
                view.runJavaScript("Notification.requestPermission()")
                phase = 20
            } else if (phase === 20 && browserWindow.pendingPermission) {
                const tab = browserWindow.activeTab
                browserWindow.activeTab = 0
                expect(!browserWindow.pendingPermission, "tab switch invalidates live request")
                browserWindow.activeTab = tab
                expect(!controller.capability("notifications").pending, "no stale pending badge")
                expect(controller.capability("notifications").policy === 2, "cancelled consent is denied")
                controller.setPermissionPolicy("notifications", 0)
                browserWindow.width = 1360; browserWindow.height = 900
                browserWindow.testBrowse(); bar.dismiss()
                input = {kind:"move",x:80,y:300}; phase = 8
            } else if (phase === 8) {
                const m = browserWindow.testOmnibox()
                expect(!m.editing && m.visibleKeys.length === 11 && !m.overflowVisible, "all 11 inline at wide size")
                expect(browserWindow.testAddressMatches(), "canonical address synchronized")
                expect(m.addressWidth >= 160 && m.menuX > m.downloadX, "navigation geometry")
                wideAddress = m.addressWidth
                console.info("OMNIBOX_METRICS browse-wide", JSON.stringify(m))
                screenshot = "omnibox-browse-1360"
                input = {kind:"key",key:Qt.Key_L,modifiers:Qt.ControlModifier}; phase = 9
            } else if (phase === 9) {
                const m = browserWindow.testOmnibox()
                expect(m.editing && m.visibleKeys.length === 0 && !m.overflowVisible && m.controlWidth === 0, "EDIT hides all controls")
                expect(m.addressWidth > wideAddress && m.selectedText === m.rawUrl, "Ctrl+L selects full URL and expands")
                console.info("OMNIBOX_METRICS edit-wide", JSON.stringify(m))
                screenshot = "omnibox-edit-1360"
                browserWindow.testDraft(m.rawUrl + "?uncommitted=draft/path"); controller.refresh(); phase = 30
            } else if (phase === 30) {
                const m = browserWindow.testOmnibox()
                expect(m.editing && m.input.includes("uncommitted=draft/path"), "refresh preserves editing")
                input = {kind:"key",key:Qt.Key_Escape}; phase = 31
            } else if (phase === 31) {
                expect(!browserWindow.testOmnibox().editing && browserWindow.testAddressMatches(), "Escape restores canonical URL without navigation")
                browserWindow.width = 820; browserWindow.height = 560; phase = 32
            } else if (phase === 32) {
                const m = browserWindow.testOmnibox()
                expect(!m.editing && m.visibleKeys.length < 11 && m.overflowVisible, "narrow suffix overflows")
                expect(m.addressWidth >= 160 && m.visibleKeys.length + m.overflowKeys.length === 11, "minimum address and complete capability set")
                expect(m.visibleKeys.every(key => !m.overflowKeys.includes(key)), "no overflow duplicates")
                narrowAddress = m.addressWidth; frozenKeys = m.overflowKeys.slice()
                console.info("OMNIBOX_METRICS browse-narrow", JSON.stringify(m))
                screenshot = "omnibox-browse-820"
                input = browserWindow.testAddressPoint(); phase = 33
            } else if (phase === 33) {
                const m = browserWindow.testOmnibox()
                expect(m.editing && m.addressWidth > narrowAddress && m.controlWidth === 0 && !m.overflowVisible, "mouse enters expanded narrow EDIT")
                console.info("OMNIBOX_METRICS edit-narrow", JSON.stringify(m))
                screenshot = "omnibox-edit-820"
                input = {kind:"key",key:Qt.Key_Escape}; phase = 34
            } else if (phase === 34) {
                input = bar.testInputPoint("overflow", "click"); phase = 35
            } else if (phase === 35) {
                expect(bar.testOverflow().visible, "overflow opens by mouse")
                expect(JSON.stringify(bar.testOverflow().keys) === JSON.stringify(frozenKeys), "only hidden suffix in menu")
                view.runJavaScript("fetch('/menu-refresh').then(()=>document.title='Menu refreshed')")
                screenshot = "omnibox-overflow"; phase = 36
            } else if (phase === 36 && view.title === "Menu refreshed") {
                expect(bar.testOverflow().visible && JSON.stringify(bar.testOverflow().keys) === JSON.stringify(frozenKeys), "network refresh keeps menu stable")
                input = {kind:"key",key:Qt.Key_Down}; phase = 37
            } else if (phase === 37) {
                expect(bar.testOverflow().currentIndex >= 0, "menu keyboard focus")
                chosenKey = frozenKeys[bar.testOverflow().currentIndex]
                input = {kind:"key",key:Qt.Key_Return}; phase = 38
            } else if (phase === 38) {
                expect(!bar.testOverflow().visible && bar.testCard().visible && bar.selectedKey === chosenKey, "overflow shares details host")
                const c = bar.testCard()
                expect(c.x >= 0 && c.y >= 72 && c.x+c.width <= browserWindow.width && c.y+c.height <= browserWindow.height, "details inside window below navigation")
                screenshot = "omnibox-details"
                input = {kind:"key",key:Qt.Key_Escape}; phase = 39
            } else if (phase === 39) {
                expect(!bar.testCard().visible, "Escape closes details first")
                bar.testFocus(); input = {kind:"key",key:Qt.Key_Right}; phase = 40
            } else if (phase === 40) {
                expect(bar.testFocusedKey() === "notifications", "roving arrow focus")
                input = bar.testInputPoint("location", "move"); phase = 41
            } else if (phase === 41 && bar.testCard().visible) {
                const c = bar.testCard()
                input = {kind:"move",x:c.x+40,y:c.y+80}; phase = 42
            } else if (phase === 42) {
                expect(bar.testCard().visible, "hover corridor into shared details")
                input = {kind:"move",x:80,y:browserWindow.height-40}; phase = 43
            } else if (phase === 43) {
                expect(!bar.testCard().visible, "mouse-out closes hover details")
                input = bar.testInputPoint("location", "click"); phase = 44
            } else if (phase === 44) {
                expect(bar.testCard().visible && bar.testCard().pinned, "click pins visible icon details")
                input = {kind:"key",key:Qt.Key_Escape}; phase = 45
            } else if (phase === 45) {
                expect(!bar.testCard().visible && browserWindow.header.height === 74, "two 36px rows plus progress line")
                browserWindow.testControlBudget = 0; phase = 46
            } else if (phase === 46) {
                const m = browserWindow.testOmnibox()
                expect(m.visibleKeys.length === 0 && m.overflowVisible && m.overflowKeys.length === 11, "zero icons budget keeps all controls in overflow")
                console.info("OMNIBOX_METRICS zero-budget", JSON.stringify(m))
                browserWindow.testControlBudget = -1
                browserWindow.addTab(""); browserWindow.testBrowse(); phase = 47
            } else if (phase === 47) {
                const m = browserWindow.testOmnibox()
                expect(m.displayHost === "" && m.input === "" && controller.capability("camera").policy === 3, "neutral blank page")
                screenshot = "omnibox-blank"
                browserWindow.openLocalFile(browserWindow.testLocalTextFile()); phase = 48
            } else if (phase === 48 && !view.loading && view.url.toString().startsWith("file:")) {
                expect(browserWindow.testOmnibox().displayHost.startsWith("file:"), "local file has no fake hostname")
                screenshot = "omnibox-file"
                browserWindow.activeTab = 0; browserWindow.testBrowse()
                controller.setBehavior("javascript", 1)
                browserWindow.applyViewPolicy(browserWindow.currentView, browserWindow.currentView.url)
                browserWindow.currentView.reload(); phase = 49
            } else if (phase === 49 && view.title === "Scripts enabled" && !view.loading) {
                browserStore.controlBarLanguage = "ru"; controller.refresh()
                expect(controller.capability("location").title === "Геолокация", "Russian catalog")
                bar.testOpen("location", true); phase = 16
            } else if (phase === 16) {
                screenshot = "omnibox-ru"; phase = 17
            } else if (phase === 17) {
                bar.dismiss(); browserStore.controlBarLanguage = oldLanguage
                expect(browserWindow.testOverlayVisibility(), "drawers/fullscreen dismiss details")
                phase = 22
            } else if (phase === 22) {
                browserWindow.currentView.runJavaScript("document.body.addEventListener('click',()=>document.title='Page clicked')")
                phase = 24
            } else if (phase === 24) {
                input = {kind:"click",x:100,y:300}; phase = 23
            } else if (phase === 23 && view.title === "Page clicked") {
                console.info("CONTROL_BAR_TEST PASS", "elapsedMs", Date.now() - started)
                phase = 18; return 2
            }
            return phase === 18 ? 2 : 0
        } catch (error) { console.error("CONTROL_BAR_TEST FAIL", phase, error); return -1 }
    }
    function takeScreenshot() { const name = screenshot; screenshot = ""; return name }
    function takeInput() { const value = input; input = null; return value }
}
