import QtQuick
import QtWebEngine

QtObject {
    required property var browserWindow
    required property var controller
    required property var bar
    property int phase: 0
    property int positionIndex: 0
    property string screenshot: ""
    property var input: null
    property string oldPosition: "right"
    property string siteA: ""
    property string siteB: ""
    property int beforeTabs: 0
    property real started: 0
    property string oldLanguage: "fr"
    function begin() {
        oldPosition = browserStore.controlBarPosition
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
                browserWindow.width = 820; browserWindow.height = 560
                phase = 8
            } else if (phase === 8 && positionIndex < 4) {
                browserStore.controlBarPosition = ["top", "right", "bottom", "left"][positionIndex]
                phase = 9
            } else if (phase === 9) {
                const geometry = bar.testGeometry()
                expect(geometry.x >= 5 && geometry.y >= 0 && geometry.x + geometry.width <= geometry.viewportWidth - 5 && geometry.y + geometry.height <= geometry.viewportHeight, "rail inside web viewport")
                expect((bar.vertical ? geometry.width : geometry.height) === 36, "thin rail")
                expect(view.width === bar.width && view.height === bar.height, "overlay does not resize page")
                expect(browserWindow.testAddressMatches(), "address reflects actual active view")
                screenshot = "controlbar-" + browserStore.controlBarPosition
                positionIndex++; phase = positionIndex < 4 ? 8 : 10
            } else if (phase === 10) {
                browserStore.controlBarPosition = "right"
                bar.testFocus(); input = {kind:"key",key:Qt.Key_Down}; phase = 21
            } else if (phase === 21) {
                expect(bar.testFocusedKey() === "notifications", "roving arrow focus")
                input = bar.testInputPoint("location", "move"); phase = 11
            } else if (phase === 11 && bar.testCard().visible) {
                const card = bar.testCard()
                input = {kind:"move",x:card.x + 40,y:card.y + 40 + browserWindow.header.height}; phase = 12
            } else if (phase === 12) {
                expect(bar.testCard().visible, "hover bridge into card")
                input = {kind:"move",x:80,y:browserWindow.height - 40}; phase = 13
            } else if (phase === 13) {
                expect(!bar.testCard().visible, "mouse-out closes unpinned card")
                input = bar.testInputPoint("network", "click"); phase = 14
            } else if (phase === 14) {
                expect(bar.testCard().visible && bar.testCard().pinned, "click pins card")
                const card = bar.testCard()
                expect(card.y >= 0 && card.y + card.height <= bar.height, "popover clamped inside viewport")
                screenshot = "controlbar-detail"
                input = {kind:"key",key:Qt.Key_Escape}; phase = 15
            } else if (phase === 15) {
                expect(!bar.testCard().visible, "escape dismisses card")
                expect(browserWindow.header.height === 74, "browser chrome unchanged")
                input = bar.testInputPoint("position", "click"); phase = 24
            } else if (phase === 24) {
                expect(bar.testPickerVisible(), "last icon opens position picker")
                input = bar.testPickerChoice(); phase = 25
            } else if (phase === 25) {
                expect(browserStore.controlBarPosition === "top" && !bar.testPickerVisible(), "position choice by mouse")
                browserStore.controlBarPosition = "right"
                browserStore.controlBarLanguage = "ru"; controller.refresh()
                expect(controller.capability("location").title === "Геолокация", "Russian catalog")
                bar.testOpen("location", true); phase = 16
            } else if (phase === 16) {
                screenshot = "controlbar-ru"; phase = 17
            } else if (phase === 17) {
                bar.dismiss()
                browserStore.controlBarLanguage = oldLanguage
                browserWindow.activeTab = 0
                controller.setBehavior("javascript", 1)
                browserWindow.applyViewPolicy(browserWindow.currentView, browserWindow.currentView.url)
                expect(browserWindow.testOverlayVisibility(), "drawers/fullscreen hide rail")
                phase = 22
            } else if (phase === 22) {
                browserWindow.currentView.runJavaScript("document.body.addEventListener('click',()=>document.title='Page clicked')")
                input = {kind:"click",x:100,y:300}; phase = 23
            } else if (phase === 23 && view.title === "Page clicked") {
                browserStore.controlBarPosition = oldPosition
                console.info("CONTROL_BAR_TEST PASS", "elapsedMs", Date.now() - started)
                phase = 18; return 2
            }
            return phase === 18 ? 2 : 0
        } catch (error) { console.error("CONTROL_BAR_TEST FAIL", phase, error); return -1 }
    }
    function takeScreenshot() { const name = screenshot; screenshot = ""; return name }
    function takeInput() { const value = input; input = null; return value }
}
