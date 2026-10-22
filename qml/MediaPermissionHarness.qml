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
    property bool noAllowPrompted: false
    function expect(value, text) { if (!value) throw new Error(text) }
    function takeScreenshot() { return "" }
    function takeInput() { const value = input; input = null; return value }
    readonly property bool deviceMode: typeof mediaDeviceMode !== "undefined" && mediaDeviceMode
    property var os: ({})
    function begin() { openTab(); phase = deviceMode ? 100 : 0 }
    function osState() { return {webcam: osCapture.state("webcam"), microphone: osCapture.state("microphone")} }
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
    function frameOrigin() { return browserStore.origin(smokeBaseUrl.replace("127.0.0.1", "localhost")) }
    // Asks the framed page for its track states through the top page, then continues in the next phase.
    function readFrame(view) {
        reading = true; state = ({})
        view.runJavaScript("window.cybouAskState()")
        frameTimer.start()
    }
    Timer {
        id: frameTimer; interval: 300; repeat: true
        onTriggered: browserWindow.currentView.runJavaScript("window.frameState", function(text) {
            if (!text) return
            frameTimer.stop(); state = JSON.parse(text); reading = false
        })
    }
    function revoke(view, key) { browserWindow.revokePermissionGracefully(view, key, 0); since = Date.now() }
    function step() {
        try {
            const view = browserWindow.currentView
            // --media-device-test: real camera and microphone, checked against what Windows records for the engine.
            if (deviceMode) {
                if (phase === 100 && ready(view)) {
                    os = osState()
                    expect(!os.webcam.inUse && !os.microphone.inUse, "precondition: Windows shows no CYBOU capture: " + JSON.stringify(os))
                    tabUid = view.tabUid; view.runJavaScript("window.cybouStart('both')"); since = Date.now(); phase = 101
                } else if (phase === 101 && browserWindow.pendingPermission) {
                    expect(browserWindow.resolvePermission(true), "grant accepted"); phase = 102
                } else if (phase === 102 && settled(view.title)) {
                    expect(view.title === "media-live", "the real devices give live tracks, got " + view.title)
                    view.runJavaScript("JSON.stringify(window.cybouStream.getTracks().map(t=>({kind:t.kind,label:t.label,state:t.readyState,settings:t.getSettings()})))",
                                       function(text) { console.info("MEDIA_DEVICE_TRACKS", text) })
                    since = Date.now(); phase = 103
                } else if (phase === 103 && elapsed() >= 3000) {
                    os = osState()
                    console.info("MEDIA_DEVICE_OS_WHILE_LIVE", JSON.stringify(os))
                    expect(os.webcam.inUse && os.microphone.inUse, "Windows records the camera and microphone in use by the engine: " + JSON.stringify(os))
                    revoke(view, "camera"); phase = 104
                } else if (phase === 104 && elapsed() >= 3000) {
                    os = osState()
                    console.info("MEDIA_DEVICE_OS_AFTER_REVOKE", JSON.stringify(os))
                    expect(!os.webcam.inUse && !os.microphone.inUse, "Windows still shows a device in use after revoke: " + JSON.stringify(os))
                    console.info("MEDIA_DEVICE_TEST PASS", "real camera and microphone live and recorded in use by Windows; after the Control Bar revoke Windows records both stopped")
                    phase = 105; return 2
                } else if (phase !== 105 && elapsed() > 40000) {
                    throw new Error("timeout in phase " + phase + " title " + view.title)
                }
                return phase === 105 ? 2 : 0
            }
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
                const screen = controller.capability("screen")
                expect(screen.blocked && screen.events.length === 1 && screen.events[0].blocked && !screen.events[0].pending,
                       "the refused screen capture is recorded once as blocked in the Control Bar: " + JSON.stringify(screen.events))
                expect(screen.events[0].origin === browserStore.origin(smokeBaseUrl), "the attempt names the requesting site")
                results.push("screen: refused without a prompt (" + view.title + ", " + "cancelled by CYBOU, recorded as blocked in the Screen control)")
                browserWindow.addTab(smokeBaseUrl + "/media-top?allow"); since = Date.now(); phase = 60
            // Cross-origin iframe (localhost framed by 127.0.0.1) with allow="camera;microphone": grant and revoke.
            } else if (phase === 60 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 61
            } else if (phase === 61 && browserWindow.pendingPermission) {
                const camera = controller.capability("camera")
                expect(camera.pending, "iframe camera request is pending in the Control Bar")
                expect(camera.requester === frameOrigin() && camera.origin === browserStore.origin(smokeBaseUrl),
                       "the prompt names the framed requester and the top site: " + camera.requester + " in " + camera.origin)
                expect(browserWindow.resolvePermission(true), "iframe grant accepted"); phase = 62
            } else if (phase === 62 && settled(view.title)) {
                expect(view.title === "media-live", "iframe grant yields a live track, got " + view.title)
                expect(view.grantedKeys.includes("camera"), "iframe grant is a lease of the tab")
                revoke(view, "camera"); phase = 63
            } else if (phase === 63 && elapsed() >= 1500) {
                if (view.tabUid !== tabUid) {
                    // The document was replaced: the old frame and its stream are gone. The new document must not be live.
                    expect(!view.grantedKeys.includes("camera"), "replacement document has no camera lease")
                    state = ({video: "document-replaced"}); phase = 65; return 0
                }
                readFrame(view); phase = 64
            } else if (phase === 65 && ready(view)) {
                expect(view.title === "media-ready", "replacement document starts without a stream")
                phase = 64
            } else if (phase === 64 && !reading) {
                expect(state.video === "ended" || state.video === "document-replaced", "iframe capture survives revoke: track is " + state.video)
                expect(!view.grantedKeys.includes("camera"), "iframe revoke removes the lease")
                results.push("iframe: requester " + frameOrigin() + " named in the prompt, grant live, revoke " + state.video)
                browserWindow.addTab(smokeBaseUrl + "/media-top?allow"); since = Date.now(); phase = 70

            // Top-level deny cascades to the cross-origin iframe, without a prompt.
            } else if (phase === 70 && ready(view)) {
                tabUid = view.tabUid
                expect(controller.setPermissionPolicy("camera", 2), "top site camera set to blocked")
                view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 71
            } else if (phase === 71) {
                if (browserWindow.pendingPermission) throw new Error("top-level deny still prompted for the iframe")
                if (!settled(view.title) && elapsed() < 6000) return 0
                expect(settled(view.title) && view.title !== "media-live", "top-level deny: no stream in the iframe, got " + view.title)
                const events = controller.capability("camera").events
                expect(events.some(e => e.blocked && e.requester === frameOrigin()), "the cascaded refusal is recorded for the framed requester: " + JSON.stringify(events))
                controller.setPermissionPolicy("camera", 0)
                results.push("iframe under top-level deny: refused (" + view.title + "), no prompt, recorded")
                browserWindow.addTab(smokeBaseUrl + "/media-top"); since = Date.now(); phase = 80

            // Iframe without an allow attribute: the engine's permissions policy refuses before CYBOU is asked.
            } else if (phase === 80 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 81
            } else if (phase === 81) {
                // The engine still asks the embedder for a frame without allow="camera", then enforces the permissions
                // policy itself. Grant the prompt to prove that even a grant yields no stream.
                if (browserWindow.pendingPermission && !noAllowPrompted) {
                    noAllowPrompted = true
                    expect(controller.capability("camera").requester === frameOrigin(), "prompt is for the framed requester")
                    browserWindow.resolvePermission(true); since = Date.now(); return 0
                }
                if (!settled(view.title) && elapsed() < 6000) return 0
                expect(settled(view.title) && view.title !== "media-live", "iframe without allow got a stream: " + view.title)
                console.info("MEDIA_NOALLOW_RESULT", view.title, "prompted", noAllowPrompted, "leases", JSON.stringify(view.grantedKeys))
                results.push("iframe without allow: no stream even when granted (" + view.title + ")"
                             + (noAllowPrompted ? ", but CYBOU showed a prompt" + (view.grantedKeys.includes("camera") ? " and lists camera as granted" : "") : ", no prompt"))
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
