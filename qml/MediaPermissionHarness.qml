import QtQuick
// Engine-level proof for media permissions (cybou_tests --media-test, CYB-PERM-001/003/004).
// Runs in the product window and drives the product's own functions (the tab's permission request reaches the
// coordinator as in normal use; resolvePermission and revokePermissionGracefully are the Control Bar actions).
// The fixture page asks for real streams (Chromium fake camera and microphone) and reports what the engine gave it.
// It is hostile on revoke: it keeps the stream in a variable, attaches it to no element and replaces
// MediaStreamTrack.prototype.stop, so only a stop the page cannot intercept ends the capture.
// Scenarios, each in a fresh tab: camera (deny, then grant and revoke), microphone, camera with microphone revoked
// one key at a time, and screen capture, which CYBOU refuses permanently.
Item {
    required property var browserWindow
    required property var coordinator
    required property var controller
    property int phase: 0
    onPhaseChanged: console.info("MEDIA_PHASE", phase)
    property real since: 0
    property string tabUid: ""
    property string refusal: ""
    property var state: ({})       // kind -> readyState read back from the page
    property bool reading: false
    property var results: []
    property var input: null
    property bool screenPending: false
    property bool activationRead: false
    property bool activated: false
    function expect(value, text) { if (!value) throw new Error(text) }
    function takeScreenshot() { return "" }
    function takeInput() { const value = input; input = null; return value }
    function begin() { openTab(); phase = 0 }
    function openTab() {
        browserWindow.addTab(smokeBaseUrl + "/media")
        since = Date.now()
    }
    function ready(view) { return view.tabUid !== tabUid && view.title === "media-ready" && !view.loading }
    function settled(title) { return title.startsWith("media-") && title !== "media-ready" && title !== "media-loading" && title !== "media-again" }
    function elapsed() { return Date.now() - since }
    // Reads back the state of the given track kinds, then continues in the next phase.
    function readTracks(view, kinds) {
        reading = true; state = ({})
        view.runJavaScript("JSON.stringify({video:window.cybouTrackState('video'),audio:window.cybouTrackState('audio')})",
                           function(text) { const all = JSON.parse(text); const next = {}; for (const k of kinds) next[k] = all[k]; state = next; reading = false })
    }
    function revoke(view, key) { browserWindow.revokePermissionGracefully(view, key, 0); since = Date.now() }
    function step() {
        try {
            const view = browserWindow.currentView
            // Camera: pending, deny, then grant in a new tab and revoke a hidden capture.
            if (phase === 0 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 1
            } else if (phase === 1 && browserWindow.pendingPermission) {
                expect(controller.capability("camera").pending, "camera request is pending in the Control Bar")
                expect(!controller.capability("mic").pending, "a camera-only request does not ask for the microphone")
                expect(view.title === "media-ready", "no stream before a decision")
                expect(!browserWindow.resolvePermission(false), "deny never reports a grant")
                phase = 2
            } else if (phase === 2 && settled(view.title)) {
                // The engine reports an embedder denial as AbortError ("Invalid state"); what matters is no stream.
                refusal = view.title
                expect(view.title !== "media-live", "explicit deny still produced a stream")
                expect(!view.grantedKeys.includes("camera") && controller.capability("camera").blocked, "deny: no lease, recorded as blocked")
                openTab(); phase = 3
            } else if (phase === 3 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('video')"); phase = 4
            } else if (phase === 4 && browserWindow.pendingPermission) {
                expect(browserWindow.resolvePermission(true), "camera grant accepted"); phase = 5
            } else if (phase === 5 && settled(view.title)) {
                expect(view.title === "media-live", "camera grant yields a live track, got " + view.title)
                expect(view.grantedKeys.includes("camera") && !view.grantedKeys.includes("mic"), "camera lease only")
                expect(browserWindow.activeProfile.listAllPermissions().length === 0, "grant is not stored as a persistent permission")
                revoke(view, "camera"); phase = 6
            } else if (phase === 6 && elapsed() >= 1500) {
                readTracks(view, ["video"]); phase = 7
            } else if (phase === 7 && !reading) {
                expect(view.tabUid === tabUid, "camera revoke keeps the document")
                expect(state.video === "ended", "camera capture survives revoke: track is " + state.video)
                expect(!view.grantedKeys.includes("camera"), "camera revoke removes the lease")
                view.runJavaScript("document.title='media-again';window.cybouStart('video')"); since = Date.now(); phase = 8
            } else if (phase === 8 && (browserWindow.pendingPermission || settled(view.title) || elapsed() > 6000)) {
                expect(view.title !== "media-live", "page regained the camera without a new decision")
                if (browserWindow.pendingPermission) browserWindow.resolvePermission(false)
                results.push("camera: pending, deny (" + refusal + "), grant live, revoke ended, no silent regrant")
                openTab(); phase = 20

            // Microphone alone.
            } else if (phase === 20 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('audio')"); phase = 21
            } else if (phase === 21 && browserWindow.pendingPermission) {
                expect(controller.capability("mic").pending && !controller.capability("camera").pending, "microphone-only request")
                expect(browserWindow.resolvePermission(true), "microphone grant accepted"); phase = 22
            } else if (phase === 22 && settled(view.title)) {
                expect(view.title === "media-live", "microphone grant yields a live track, got " + view.title)
                expect(view.grantedKeys.includes("mic") && !view.grantedKeys.includes("camera"), "microphone lease only")
                revoke(view, "mic"); phase = 23
            } else if (phase === 23 && elapsed() >= 1500) {
                readTracks(view, ["audio"]); phase = 24
            } else if (phase === 24 && !reading) {
                expect(state.audio === "ended", "microphone capture survives revoke: track is " + state.audio)
                expect(!view.grantedKeys.includes("mic"), "microphone revoke removes the lease")
                results.push("microphone: grant live, revoke ended")
                openTab(); phase = 30

            // Camera with microphone in one request, revoked one key at a time.
            } else if (phase === 30 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('both')"); phase = 31
            } else if (phase === 31 && browserWindow.pendingPermission) {
                expect(controller.capability("camera").pending && controller.capability("mic").pending, "one request shows both camera and microphone pending")
                expect(browserWindow.resolvePermission(true), "combined grant accepted"); phase = 32
            } else if (phase === 32 && settled(view.title)) {
                expect(view.title === "media-live", "combined grant yields live tracks, got " + view.title)
                expect(view.grantedKeys.includes("camera") && view.grantedKeys.includes("mic"), "combined lease holds both keys")
                readTracks(view, ["video", "audio"]); phase = 33
            } else if (phase === 33 && !reading) {
                expect(state.video === "live" && state.audio === "live", "both tracks live before revoke: " + JSON.stringify(state))
                revoke(view, "camera"); phase = 34
            } else if (phase === 34 && elapsed() >= 1500) {
                readTracks(view, ["video", "audio"]); phase = 35
            } else if (phase === 35 && !reading) {
                console.info("MEDIA_COMBINED_AFTER_CAMERA_REVOKE", JSON.stringify(state), "leases", JSON.stringify(view.grantedKeys))
                expect(state.video === "ended", "camera capture survives a camera revoke of a combined grant: " + state.video)
                expect(!view.grantedKeys.includes("camera"), "camera key leaves the combined lease")
                // The engine backs a combined grant with one decision: revoking the camera ends the microphone too.
                // What must hold is that the Control Bar never shows a device as granted while its track has stopped.
                // The Control Bar must not show the microphone as granted while the engine has already stopped it.
                expect(state.audio === "live" || !view.grantedKeys.includes("mic"),
                       "microphone shown as granted but its track is " + state.audio)
                const micAfterCamera = state.audio
                revoke(view, "mic"); phase = 36
                results.push("camera+microphone: one prompt, both live; camera revoke ended video, microphone " + micAfterCamera)
            } else if (phase === 36 && !view.grantedKeys.includes("mic") && state.audio === "ended") {
                // The microphone already ended with the shared grant: nothing is left to revoke.
                results[results.length - 1] += "; microphone lease ended with the shared grant"
                openTab(); phase = 40
            } else if (phase === 36 && elapsed() >= 1500) {
                readTracks(view, ["video", "audio"]); phase = 37
            } else if (phase === 37 && !reading) {
                expect(state.video === "ended" && state.audio === "ended", "both tracks end after both revokes: " + JSON.stringify(state))
                expect(!view.grantedKeys.includes("camera") && !view.grantedKeys.includes("mic"), "no lease left")
                results[results.length - 1] += "; microphone revoke ended audio"
                openTab(); phase = 40

            // Screen capture: refused by policy, with a real click as the user gesture getDisplayMedia needs.
            } else if (phase === 40 && ready(view)) {
                tabUid = view.tabUid
                input = {kind: "click", x: browserWindow.width / 2, y: browserWindow.height / 2 + 60}
                since = Date.now(); phase = 41
            } else if (phase === 41) {
                if (browserWindow.pendingPermission) screenPending = true
                if (!settled(view.title) && elapsed() < 8000) {
                    if (view.title === "media-ready" && elapsed() > 2500 && elapsed() < 2700)
                        input = {kind: "click", x: browserWindow.width / 2, y: browserWindow.height / 2 + 60}
                    return 0
                }
                if (!activationRead) {
                    activationRead = true; reading = true
                    view.runJavaScript("navigator.userActivation.hasBeenActive", function(v) { activated = v === true; reading = false })
                    return 0
                }
                if (reading) return 0
                console.info("MEDIA_SCREEN_RESULT", view.title, "userActivation", activated, "events", JSON.stringify(controller.capability("screen").events))
                // Without a click getDisplayMedia fails for lack of a gesture, which would prove nothing about CYBOU.
                expect(activated, "the click did not reach the page: no user activation")
                expect(settled(view.title), "screen request never settled: " + view.title)
                expect(view.title !== "media-live", "screen capture produced a stream")
                expect(!screenPending, "screen capture was offered for a decision")
                expect(!view.grantedKeys.includes("screen"), "no screen lease")
                results.push("screen: refused without a prompt (" + view.title + ", " + (controller.capability("screen").blocked ? "recorded as blocked" : "cancelled by Qt: CYBOU handles no desktopMediaRequested") + ")")
                phase = 50
            } else if (phase === 50) {
                console.info("MEDIA_PERMISSION_TEST PASS", results.join(" | "))
                phase = 51; return 2
            } else if (phase !== 51 && elapsed() > 40000) {
                throw new Error("timeout in phase " + phase + " title " + view.title)
            }
            return phase === 51 ? 2 : 0
        } catch (error) { console.error("MEDIA_PERMISSION_TEST FAIL", phase, error); return -1 }
    }
}
