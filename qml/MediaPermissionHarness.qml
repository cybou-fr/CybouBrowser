import QtQuick
// Engine-level proof for camera permissions (cybou_tests --media-test, CYB-PERM-001/003/004).
// Runs in the product window and drives the product's own functions (the tab's permission request reaches the
// coordinator as in normal use; resolvePermission and revokePermissionGracefully are the Control Bar actions).
// The fixture page asks for a real camera stream (Chromium fake device) and reports what the engine gave it.
// It is hostile on revoke: it keeps the stream in a variable, attaches it to no element and replaces
// MediaStreamTrack.prototype.stop, so only a stop the page cannot intercept ends the capture.
Item {
    required property var browserWindow
    required property var coordinator
    required property var controller
    property int phase: 0
    onPhaseChanged: console.info("MEDIA_PHASE", phase)
    property real since: 0
    property string denyTabUid: ""
    property string revokedTabUid: ""
    property string trackState: ""
    property string refusal: ""
    function expect(value, text) { if (!value) throw new Error(text) }
    function takeScreenshot() { return "" }
    function takeInput() { return null }
    function begin() {
        browserWindow.navigate(smokeBaseUrl + "/media")
        since = Date.now(); phase = 0
    }
    function settled(title) { return title.startsWith("media-") && title !== "media-ready" && title !== "media-loading" }
    function elapsed() { return Date.now() - since }
    function step() {
        try {
            const view = browserWindow.currentView
            if (phase === 0 && view.title === "media-ready" && !view.loading) {
                view.runJavaScript("window.cybouStart()"); since = Date.now(); phase = 1
            } else if (phase === 1 && browserWindow.pendingPermission) {
                // Precondition: a camera request is held for a decision, nothing is granted meanwhile.
                expect(controller.capability("camera").pending, "camera request is pending in the Control Bar")
                expect(view.title === "media-ready", "no stream before a decision")
                expect(!browserWindow.resolvePermission(false), "deny never reports a grant")
                denyTabUid = view.tabUid; phase = 2
            } else if (phase === 2 && settled(view.title)) {
                // The engine reports an embedder denial as AbortError ("Invalid state"); what matters is no stream.
                refusal = view.title
                expect(view.title !== "media-live", "explicit deny still produced a stream")
                expect(!view.grantedKeys.includes("camera"), "no lease after deny")
                expect(controller.capability("camera").blocked, "deny recorded in the Control Bar")
                browserWindow.addTab(smokeBaseUrl + "/media"); since = Date.now(); phase = 3
            } else if (phase === 3 && view.tabUid !== denyTabUid && view.title === "media-ready" && !view.loading) {
                view.runJavaScript("window.cybouStart()"); phase = 4
            } else if (phase === 4 && browserWindow.pendingPermission) {
                expect(browserWindow.resolvePermission(true), "grant accepted")
                phase = 5
            } else if (phase === 5 && settled(view.title)) {
                expect(view.title === "media-live", "grant yields a live camera track, got " + view.title)
                expect(view.grantedKeys.includes("camera"), "grant is tracked as a lease")
                expect(browserWindow.activeProfile.listAllPermissions().length === 0, "grant is not stored as a persistent permission")
                // Revoke exactly as the Control Bar does for the camera, while the page hides the stream.
                revokedTabUid = view.tabUid
                browserWindow.revokePermissionGracefully(view, "camera", 0)
                since = Date.now(); phase = 6
            } else if (phase === 6 && elapsed() >= 1500) {
                if (view.tabUid !== revokedTabUid) { trackState = "document-replaced"; phase = 8; return 0 }
                trackState = ""
                view.runJavaScript("window.cybouTrackState()", function(state) { trackState = String(state) })
                phase = 7
            } else if (phase === 7 && trackState !== "") {
                console.info("MEDIA_REVOKE_TRACK_STATE", trackState)
                expect(trackState === "ended", "camera capture survives revoke: the page's track is still " + trackState)
                phase = 8
            } else if (phase === 8) {
                expect(!view.grantedKeys.includes("camera"), "revoke removes the camera lease")
                if (view.tabUid !== revokedTabUid) { phase = 10; return 0 }
                // After revoke the page must not get the camera back without a new decision.
                view.runJavaScript("document.title='media-again';window.cybouStart()")
                since = Date.now(); phase = 9
            } else if (phase === 9 && (browserWindow.pendingPermission || settled(view.title) && view.title !== "media-again" || elapsed() > 6000)) {
                expect(view.title !== "media-live", "page regained the camera without a new decision")
                if (browserWindow.pendingPermission) browserWindow.resolvePermission(false)
                phase = 10
            } else if (phase === 10) {
                console.info("MEDIA_PERMISSION_TEST PASS", "pending until decided; deny gives no stream (" + refusal + "); grant gives a live track held as a non-persistent lease; revoke ends a hidden capture (" + trackState + "); no silent regrant")
                phase = 11; return 2
            } else if (phase !== 11 && elapsed() > 40000) {
                throw new Error("timeout in phase " + phase + " title " + view.title)
            }
            return phase === 11 ? 2 : 0
        } catch (error) { console.error("MEDIA_PERMISSION_TEST FAIL", phase, error); return -1 }
    }
}
