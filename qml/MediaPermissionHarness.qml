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
    property double staleId: 0
    property int backgroundIndex: 0
    readonly property bool noDeviceMode: typeof mediaNoDeviceMode !== "undefined" && mediaNoDeviceMode
    // CYB-PERM-006: the Control Bar records grants, never observed recording. It must not expose any field or wording
    // that claims a device is capturing, whether or not a stream exists.
    function expectNoCaptureClaim(capability, when) {
        for (const key of ["active", "recording", "inUse", "capturing", "live"])
            expect(!(key in capability) || !capability[key], when + ": the Control Bar claims capture through '" + key + "'")
        expect(capability.reason === qsTr("Utilisation réelle non observable. Aucun indicateur d’enregistrement."),
               when + ": the camera control does not state that real use is unobservable: " + capability.reason)
    }
    property int tabCount: 0
    property var requestView: null
    property var lifecycle: []
    property bool noAllowPrompted: false
    property bool noDevicePrompted: false
    function expect(value, text) { if (!value) throw new Error(text) }
    function takeScreenshot() { return "" }
    function takeInput() { const value = input; input = null; return value }
    readonly property bool deviceMode: typeof mediaDeviceMode !== "undefined" && mediaDeviceMode
    property var os: ({})
    function begin() { openTab(); phase = deviceMode ? 100 : noDeviceMode ? 200 : 0 }
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
            // --media-nodevice-test: the engine has no camera (fake device list empty). The request must end without a
            // stream, nothing may claim capture, and a grant, if asked, stays a grant of permission only.
            if (noDeviceMode) {
                if (phase === 200 && ready(view)) {
                    tabUid = view.tabUid; view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 201
                } else if (phase === 201) {
                    if (browserWindow.pendingPermission && !noDevicePrompted) { noDevicePrompted = true; browserWindow.resolvePermission(true) }
                    if (!settled(view.title) && elapsed() < 20000) return 0
                    expect(settled(view.title), "the request never ended: " + view.title)
                    expect(view.title !== "media-live", "a stream exists without a device")
                    expectNoCaptureClaim(controller.capability("camera"), "no device")
                    console.info("MEDIA_NODEVICE_TEST PASS", "no camera: request ended with " + view.title + ", prompted " + noDevicePrompted
                                 + ", lease " + view.grantedKeys.includes("camera") + "; the Control Bar claims no capture")
                    phase = 202; return 2
                }
                return phase === 202 ? 2 : 0
            }
            // --media-device-test: real camera and microphone, checked against what Windows records for the engine.
            if (deviceMode) {
                if (phase === 100 && ready(view)) {
                    // The CapabilityAccessManager registry is logged only: on Windows build 26300 it recorded no use by any
                    // application while the camera indicator was lit (2026-10-22), so it is no evidence either way.
                    console.info("MEDIA_DEVICE_OS_BEFORE", JSON.stringify(osState()))
                    tabUid = view.tabUid; view.runJavaScript("window.cybouStart('both')"); since = Date.now(); phase = 101
                } else if (phase === 101 && browserWindow.pendingPermission) {
                    expect(browserWindow.resolvePermission(true), "grant accepted"); phase = 102
                } else if (phase === 102 && settled(view.title)) {
                    expect(view.title === "media-live", "the real devices give live tracks, got " + view.title)
                    reading = true
                    view.runJavaScript("JSON.stringify(window.cybouStream.getTracks().map(t=>({kind:t.kind,label:t.label,state:t.readyState})))",
                                       function(text) { console.info("MEDIA_DEVICE_TRACKS", text); state = {tracks: JSON.parse(text)}; reading = false })
                    since = Date.now(); phase = 103
                } else if (phase === 103 && !reading && elapsed() >= 4000) {
                    expect(state.tracks.length === 2 && state.tracks.every(t => t.state === "live" && t.label && !/fake/i.test(t.label)),
                           "two live tracks from real devices: " + JSON.stringify(state.tracks))
                    console.info("MEDIA_DEVICE_OS_WHILE_LIVE", JSON.stringify(osState()))
                    console.info("MEDIA_DEVICE_OBSERVE revoke at", new Date().toISOString(), "- the camera indicator and the tray icon should go off now; CYBOU keeps running 8 s")
                    revoke(view, "camera"); phase = 104
                } else if (phase === 104 && elapsed() >= 1500) {
                    readTracks(view, ["video", "audio"]); phase = 106
                } else if (phase === 106 && !reading) {
                    console.info("MEDIA_DEVICE_AFTER_REVOKE", JSON.stringify(state), "os", JSON.stringify(osState()))
                    expect(state.video === "ended" && state.audio === "ended", "real camera and microphone tracks end after the revoke: " + JSON.stringify(state))
                    since = Date.now(); phase = 107
                } else if (phase === 107 && elapsed() >= 8000) {
                    console.info("MEDIA_DEVICE_TEST PASS", "real camera and microphone gave live tracks; after the Control Bar revoke both ended; CYBOU stayed open 8 s for the OS indicator to be observed")
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
                expectNoCaptureClaim(controller.capability("camera"), "live grant")
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
                // The page did not delegate the camera: CYBOU reads the frame's permissions policy and refuses at once.
                if (browserWindow.pendingPermission) throw new Error("an iframe without allow=camera was offered the camera")
                if (!settled(view.title) && elapsed() < 6000) return 0
                expect(settled(view.title) && view.title !== "media-live", "iframe without allow got a stream: " + view.title)
                expect(!view.grantedKeys.includes("camera"), "no camera shown as granted for an undelegated frame")
                const refused = controller.capability("camera").events.filter(e => e.blocked && e.requester === frameOrigin())
                expect(refused.length === 1, "the refusal is recorded once for the framed requester: " + JSON.stringify(controller.capability("camera").events))
                results.push("iframe without allow: refused by CYBOU without a prompt (" + view.title + "), recorded, nothing shown as granted")
                openTab(); phase = 90

            // Request lifecycle: a late decision never grants a document that is gone or no longer in front.
            } else if (phase === 90 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 91
            } else if (phase === 91 && browserWindow.pendingPermission) {
                staleId = coordinator.pendingRequestId
                view.url = smokeBaseUrl + "/media?navigated"; phase = 92
            } else if (phase === 92 && view.url.toString().endsWith("?navigated") && view.title === "media-ready" && !view.loading) {
                expect(!coordinator.resolve(true, view, view.profile, staleId), "a decision for the previous document granted the new one")
                expect(!browserWindow.pendingPermission && !view.grantedKeys.includes("camera"), "navigation leaves no pending request or lease")
                lifecycle.push("navigation cancels the pending request")
                openTab(); phase = 93
            } else if (phase === 93 && ready(view)) {
                tabUid = view.tabUid; view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 94
            } else if (phase === 94 && browserWindow.pendingPermission) {
                staleId = coordinator.pendingRequestId
                tabCount = browserWindow.testTabCount()
                browserWindow.closeTab(browserWindow.activeTab); phase = 95
            } else if (phase === 95 && browserWindow.testTabCount() === tabCount - 1) {
                expect(!browserWindow.pendingPermission, "closing the tab leaves no pending request")
                expect(!coordinator.resolve(true, browserWindow.currentView, browserWindow.currentView.profile, staleId), "a decision for a closed tab granted another view")
                lifecycle.push("closing the tab cancels it")
                openTab(); phase = 96
            } else if (phase === 96 && ready(view)) {
                tabUid = view.tabUid; requestView = view
                view.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 97
            } else if (phase === 97 && browserWindow.pendingPermission) {
                staleId = coordinator.pendingRequestId
                browserWindow.addTab(smokeBaseUrl + "/one"); since = Date.now(); phase = 98
            } else if (phase === 98 && view !== requestView && !view.loading) {
                // The request belongs to a background tab now: a decision taken from the front tab must not grant it.
                expect(!browserWindow.resolvePermission(true, staleId), "a decision from another tab granted the background request")
                since = Date.now(); phase = 99
            } else if (phase === 99 && elapsed() >= 1500) {
                expect(requestView.title !== "media-live", "the background tab got a stream")
                expect(!requestView.grantedKeys.includes("camera"), "the background tab holds a camera lease")
                if (browserWindow.pendingPermission) coordinator.cancel()
                lifecycle.push("a decision from another tab does not grant a background request")
                results.push("lifecycle: " + lifecycle.join(", "))
                openTab(); phase = 110

            // CYB-PERM-005: a request made by a tab in the background shows no prompt and grants nothing until the user
            // brings that tab to the front; then it is asked for normally.
            } else if (phase === 110 && ready(view)) {
                tabUid = view.tabUid; requestView = view; backgroundIndex = browserWindow.activeTab
                browserWindow.addTab(smokeBaseUrl + "/one"); phase = 111
            } else if (phase === 111 && view !== requestView && !view.loading) {
                requestView.runJavaScript("window.cybouStart('video')"); since = Date.now(); phase = 112
            } else if (phase === 112) {
                if (browserWindow.pendingPermission) throw new Error("a background tab's request was shown while another tab is in front")
                if (elapsed() < 2000) return 0
                expect(requestView.title === "media-ready" && !requestView.grantedKeys.includes("camera"), "the background tab got the camera without a decision")
                browserWindow.activeTab = backgroundIndex; since = Date.now(); phase = 113
            } else if (phase === 113 && browserWindow.pendingPermission) {
                expect(browserWindow.pendingPermissionView === requestView, "the prompt belongs to the tab brought to the front")
                expect(!browserWindow.resolvePermission(false), "deny never reports a grant")
                phase = 114
            } else if (phase === 114 && settled(requestView.title)) {
                expect(requestView.title !== "media-live", "denied background request produced a stream")
                results.push("background tab: queued without a prompt for 2 s, asked when brought to the front, denied")
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
