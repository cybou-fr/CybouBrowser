import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs as NativeDialogs
import QtWebEngine
import QtQuick.Effects
import Cybou

ApplicationWindow {
    id: window
    width: 1360; height: 900
    minimumWidth: 820; minimumHeight: 560
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    title: (currentView && currentView.title ? currentView.title + " — " : "") + "CYBOU" + (privateMode ? " — Privé" : "")
    color: CybouTheme.navyDeep
    palette.button: CybouTheme.glassSurface
    palette.buttonText: CybouTheme.mainText
    palette.text: CybouTheme.mainText
    palette.windowText: CybouTheme.mainText
    palette.base: CybouTheme.navyDeep
    palette.highlight: "#1D4ED8"
    palette.highlightedText: "#FFFFFF"
    palette.placeholderText: CybouTheme.mutedText

    component WindowDragArea: MouseArea {
        acceptedButtons: Qt.LeftButton
        property point pressPosition
        property bool moving: false
        onPressed: function(mouse) { pressPosition = Qt.point(mouse.x, mouse.y); moving = false }
        onPositionChanged: function(mouse) {
            if (pressed && !moving && Math.abs(mouse.x - pressPosition.x) + Math.abs(mouse.y - pressPosition.y) >= Qt.application.styleHints.startDragDistance)
                moving = window.startSystemMove()
        }
        onReleased: moving = false
        onCanceled: moving = false
        onDoubleClicked: window.maximizeToggle()
    }

    property int activeTab: 0
    readonly property int chromeRowHeight: CybouTheme.chromeRowHeight
    readonly property int chromeControlHeight: CybouTheme.chromeControlHeight
    property var currentView: views.count > activeTab ? views.itemAt(activeTab) : null
    property bool privateMode: false
    readonly property bool smokeActive: typeof smokeMode !== "undefined" && smokeMode
    property string initialUrlArgument: ""
    property string initialLibrarySection: ""
    property bool initialSettingsOpen: false
    property bool initialMenuOpen: false
    property bool initialCloudLauncherOpen: false
    property var closedTabs: []
    property var downloadItems: []
    property var downloadQueue: []
    property var pendingDownload: null
    property string pendingOpenPath: ""
    property string pendingOpenOrigin: ""
    function reviewDownloadedFile(item) {
        if (!item || item.state !== WebEngineDownloadRequest.DownloadCompleted) return
        pendingOpenPath = downloadPath(item)
        pendingOpenOrigin = browserStore.origin(item.url.toString())
        downloadOpenDialog.open()
    }
    property var pdfTarget: null
    property bool pdfBusy: false
    property bool closingApproved: false
    property var draggedTab: null
    property real dragPointerX: 0
    property var suggestionRows: []
    property int suggestionIndex: -1
    function updateSuggestions() {
        suggestionRows = browserStore.suggestions(address.text, !privateMode)
        suggestionIndex = -1
        if (address.activeFocus && suggestionRows.length) suggestionsPopup.open()
        else suggestionsPopup.close()
    }
    function acceptAddress() {
        const value = suggestionsPopup.visible && suggestionIndex >= 0 ? suggestionRows[suggestionIndex].url : address.text
        suggestionsPopup.close()
        navigate(value)
    }
    function pinnedCount() {
        let count = 0
        for (let i = 0; i < tabs.count; ++i) if (tabs.get(i).isPinned) count++
        return count
    }
    function togglePin(index) {
        const pin = !tabs.get(index).isPinned
        tabs.setProperty(index, "isPinned", pin)
        moveTab(index, pin ? pinnedCount() - 1 : pinnedCount(), true)
        saveTimer.restart()
    }
    function toggleMute(index) { tabs.setProperty(index, "isMuted", !tabs.get(index).isMuted); saveTimer.restart() }
    function restoreTab(entry) {
        if (typeof entry === "string") addTab(entry)
        else if (entry && entry.url) addTab(entry.url, entry)
    }
    function showStatus(message) { statusLabel.text = message; statusPopup.open(); statusTimer.restart() }
    function activeDownloads() {
        for (const item of downloadItems) if (item && !item.isFinished) return true
        return false
    }
    function downloadPath(item) { return item.downloadDirectory + "/" + item.downloadFileName }
    function downloadStatus(item) {
        if (item.state === WebEngineDownloadRequest.DownloadRequested) return "En attente du dossier…"
        if (item.state === WebEngineDownloadRequest.DownloadCompleted) return "Terminé"
        if (item.state === WebEngineDownloadRequest.DownloadCancelled) return "Annulé"
        if (item.state === WebEngineDownloadRequest.DownloadInterrupted) return item.interruptReasonString
        const received = (item.receivedBytes / 1048576).toFixed(1)
        const total = item.totalBytes > 0 ? " / " + (item.totalBytes / 1048576).toFixed(1) + " Mo" : " Mo"
        return (item.isPaused ? "En pause · " : "") + received + total
    }
    function presentNextDownload() {
        if (pendingDownload || !downloadQueue.length) return
        pendingDownload = downloadQueue[0]
        downloadQueue = downloadQueue.slice(1)
        if (!pendingDownload || pendingDownload.isFinished) { pendingDownload = null; Qt.callLater(presentNextDownload); return }
        saveDownloadDialog.currentFolder = browserStore.fileUrl(browserStore.downloadsPath)
        saveDownloadDialog.selectedFile = browserStore.fileUrl(browserStore.nextDownloadPath(browserStore.downloadsPath, pendingDownload.downloadFileName))
        saveDownloadDialog.open()
    }
    function finishDownloadChoice(file) {
        const item = pendingDownload
        pendingDownload = null
        if (item && !item.isFinished) {
            if (file && browserStore.localPath(file).length) {
                item.downloadDirectory = browserStore.fileDirectory(file)
                item.downloadFileName = browserStore.fileName(file)
                item.accept()
            } else item.cancel()
        }
        Qt.callLater(presentNextDownload)
    }
    function exportPdf() {
        if (!currentView || currentView.loading || currentView.url.toString() === "about:blank" || pdfBusy) return
        pdfTarget = currentView
        pdfDialog.currentFolder = browserStore.fileUrl(browserStore.downloadsPath)
        pdfDialog.selectedFile = browserStore.fileUrl(browserStore.downloadsPath + "/" + browserStore.safeFileName(currentView.title) + ".pdf")
        pdfDialog.open()
    }
    function openLocalFile(file) {
        const url = browserStore.openableFile(file)
        if (url) addTab(url)
        else showStatus("Ce fichier ne peut pas être ouvert dans le navigateur.")
    }
    function dragTabAt(item, x) {
        dragPointerX = x
        const position = x + tabStrip.contentX
        const targetIndex = tabStrip.indexAt(position, tabStrip.height / 2)
        if (targetIndex < 0 || targetIndex === item.index) return
        const target = tabStrip.itemAtIndex(targetIndex)
        if (target && ((targetIndex > item.index && position > target.x + target.width / 2)
            || (targetIndex < item.index && position < target.x + target.width / 2))) moveTab(item.index, targetIndex)
    }
    Timer {
        interval: 30; repeat: true; running: window.draggedTab !== null
        onTriggered: {
            const direction = dragPointerX < 24 ? -1 : dragPointerX > tabStrip.width - 24 ? 1 : 0
            if (direction) {
                tabStrip.contentX = Math.max(0, Math.min(Math.max(0, tabStrip.contentWidth - tabStrip.width), tabStrip.contentX + direction * 12))
                if (draggedTab) dragTabAt(draggedTab, dragPointerX)
            }
        }
    }
    property string librarySection: "bookmarks"
    property bool findVisible: false
    property string findResult: ""
    property bool bookmarked: false
    property bool restoring: true
    property real testControlBudget: -1
    readonly property var activeProfile: privateMode ? privateProfile : persistentProfile
    readonly property var currentProfile: currentView ? currentView.profile : activeProfile
    property var temporaryProfiles: ({})
    property int trackerExceptionRevision: 0
    Connections {
        target: browserStore
        function onTrackerExceptionsChanged() { window.trackerExceptionRevision++ }
    }
    readonly property var transportState: {
        window.trackerExceptionRevision
        const view = currentView
        const pending = !!view && view.blockedHttpUrl.length > 0 && view.blockedHttpEpoch === view.navigationEpoch
        const trackerActive = !!view && browserStore.isTrackerAllowedInProfile(view.profile, view.url)
        const adActive = !!view && browserStore.isAdAllowedInProfile(view.profile, view.url)
        return {
            httpPending: pending,
            httpOrigin: pending ? browserStore.origin(view.blockedHttpUrl) : "",
            httpToken: view ? view.tabUid + "@" + view.navigationEpoch : "",
            httpTemporary: !!view && view.temporaryTransport,
            trackerTemporary: trackerActive,
            adTemporary: adActive,
            trackerCanAllow: !!view && !view.loading && !view.temporaryContext && !trackerActive && browserStore.blockTrackers && browserStore.canRequestTemporaryTrackers(view.url),
            adCanAllow: !!view && !view.loading && !view.temporaryContext && !adActive && browserStore.blockTrackers && browserStore.canRequestTemporaryAds(view.url)
        }
    }
    function allowTemporaryTrackers(token) {
        if (!currentView || !transportState.trackerCanAllow || token !== transportState.httpToken) return false
        if (!browserStore.allowTrackersInProfile(currentView.profile, currentView.url, currentView)) return false
        currentView.reload()
        return true
    }
    function revokeTemporaryTrackers(token) {
        if (!currentView || !transportState.trackerTemporary || token !== transportState.httpToken) return false
        browserStore.revokeTrackersForOrigin(currentView.profile, currentView.url)
        currentView.reload()
        return true
    }
    function allowTemporaryAds(token) {
        if (!currentView || !transportState.adCanAllow || token !== transportState.httpToken) return false
        if (!browserStore.allowAdsInProfile(currentView.profile, currentView.url, currentView)) return false
        currentView.reload()
        return true
    }
    function revokeTemporaryAds(token) {
        if (!currentView || !transportState.adTemporary || token !== transportState.httpToken) return false
        browserStore.revokeAdsForOrigin(currentView.profile, currentView.url)
        currentView.reload()
        return true
    }
    function offerHttp(view, url) {
        if (!view || !browserStore.canRequestTemporaryHttp(url)) return
        view.blockedHttpUrl = url.toString()
        view.blockedHttpEpoch = view.navigationEpoch
        if (view === currentView) window.showStatus(qsTr("HTTP bloqué. Ouvrez le contrôle Réseau pour cette tentative."))
    }
    function allowTemporaryHttp(token) {
        if (!currentView || !transportState.httpPending || token !== transportState.httpToken) return false
        const target = currentView.blockedHttpUrl
        const profile = browserStore.createTemporaryHttpProfile(target)
        if (!profile) return false
        window.replaceGrantedView(currentView, target, profile)
        return true
    }
    function endTemporaryNavigation(view, target) {
        const uid = view.tabUid
        const epoch = view.navigationEpoch
        const offer = browserStore.canRequestTemporaryHttp(target)
        Qt.callLater(function() {
            for (let i = 0; i < views.count; ++i) {
                const owner = views.itemAt(i)
                if (!owner || owner.tabUid !== uid || owner.navigationEpoch !== epoch) continue
                const replacement = window.replaceGrantedView(owner)
                if (offer) Qt.callLater(function() {
                    for (let j = 0; j < views.count; ++j) {
                        const next = views.itemAt(j)
                        if (next && next.tabUid === replacement) { window.offerHttp(next, target); return }
                    }
                })
                return
            }
        })
    }
    function releaseTemporaryView(uid) {
        const profile = temporaryProfiles[uid]
        if (!profile) return
        const next = Object.assign({}, temporaryProfiles)
        delete next[uid]
        temporaryProfiles = next
        browserStore.releaseTemporaryProfile(profile)
    }
    readonly property var pendingPermission: permissionCoordinator.hasPending ? permissionCoordinator.pendingPermission : null
    readonly property var pendingPermissionView: permissionCoordinator.pendingView
    property int nextTabUid: 0
    function applyViewPolicy(view, target) {
        view.settings.javascriptEnabled = controlState.settingFor(target, "javascript", view.tabUid)
        view.settings.localStorageEnabled = controlState.settingFor(target, "storage", view.tabUid)
        view.settings.javascriptCanOpenWindows = controlState.popupMode(target, view.tabUid) !== 2
        view.policyNeedsReload = false
    }
    ControlBarController {
        id: controlState
        store: browserStore
        profile: window.currentProfile
        url: window.currentView ? window.currentView.url : "about:blank"
        tabId: window.currentView ? window.currentView.tabUid : ""
        javascriptEnabled: window.currentView ? window.currentView.settings.javascriptEnabled : true
        storageEnabled: window.currentView ? window.currentView.settings.localStorageEnabled : true
        grantedKeys: window.currentView ? window.currentView.grantedKeys : []
        reloadRequired: window.currentView ? window.currentView.policyNeedsReload : false
        onPolicyChanged: function(origin, key) {
            for (let i = 0; i < views.count; ++i) {
                const view = views.itemAt(i)
                if (!view || view !== currentView || browserStore.origin(view.url.toString()) !== origin) continue
                if (key === "javascript" || key === "storage") view.policyNeedsReload = true
                if (key === "popups") view.settings.javascriptCanOpenWindows = controlState.popupMode(view.url) !== 2
            }
        }
    }
    PermissionCoordinator {
        id: permissionCoordinator
        activeView: currentView
        onRequestObserved: function(permission, tab, top, blocked, requestId) {
            controlState.notePermission(permission, tab, top, blocked, requestId)
        }
        onDecisionFinished: function(permission, tab, allowed, requestId) {
            controlState.finishPermission(permission, tab, allowed, requestId)
            if (!permissionCoordinator.hasPendingFor(tab))
                controlState.abandonPending(tab)
        }
        onPermissionRevoked: function(view, key) {
            window.stopMediaTracks(view, key)
        }
        onRevokeDocument: function(view) { window.replaceGrantedView(view) }
    }
    function stopMediaTracks(view, key) {
        if (!view || typeof view.runJavaScript !== "function") return
        const targetKey = key || "all"
        const script = "(function(targetKey) {" +
            "try {" +
                "var stopTrack = function(t) {" +
                    "if (!t) return;" +
                    "if (!targetKey || targetKey === 'all' || " +
                        "(targetKey === 'camera' && t.kind === 'video') || " +
                        "(targetKey === 'mic' && t.kind === 'audio') || " +
                        "(targetKey === 'screen')) {" +
                        "t.stop();" +
                    "}" +
                "};" +
                "var els = document.querySelectorAll('video, audio');" +
                "for (var i = 0; i < els.length; i++) {" +
                    "var el = els[i];" +
                    "if (el && el.srcObject && el.srcObject.getTracks) {" +
                        "var tracks = el.srcObject.getTracks();" +
                        "for (var j = 0; j < tracks.length; j++) stopTrack(tracks[j]);" +
                    "}" +
                "}" +
            "} catch(e) {}" +
        "})(" + JSON.stringify(targetKey) + ");"
        view.runJavaScript(script)
    }
    function revokePermissionGracefully(view, key, targetMode) {
        if (!view) return
        const isDeny = (targetMode === 2)
        // A frame of another origin keeps capturing after its grant is reset: only a new document stops it for sure.
        const replaceDocument = permissionCoordinator && permissionCoordinator.hasCrossOriginLease(view, key)
        if (controlState) {
            controlState.setPermissionPolicy(key, targetMode)
        }
        if (permissionCoordinator) {
            permissionCoordinator.revokeLease(view, key, isDeny)
        }
        if (replaceDocument) {
            window.replaceGrantedView(view, view.url.toString())
            window.showStatus(qsTr("Accès révoqué. La page a été rechargée pour arrêter la capture d’un cadre intégré."))
            return
        }
        stopMediaTracks(view, key)
        window.showStatus(isDeny ? qsTr("Accès bloqué.") : qsTr("Accès révoqué."))
    }
    function replaceGrantedView(view, replacementUrl, transportProfile) {
        const targetUrl = replacementUrl === undefined ? "about:blank" : replacementUrl
        if (!view) return
        for (let i = 0; i < tabs.count; ++i) {
            if (views.itemAt(i) !== view) continue
            const pinned = tabs.get(i).isPinned
            const muted = tabs.get(i).isMuted
            const wasCurrent = view === currentView
            const selected = activeTab
            const newUid = "tab-" + (++nextTabUid)
            if (targetUrl !== "about:blank") controlState.inheritContext(view.tabUid, newUid, view.url, targetUrl)
            controlState.expireContext(view.tabUid)
            if (pdfTarget === view) { pdfTarget = null; pdfBusy = false }
            // Removing the repeater item destroys its WebEngineView; no beforeunload veto.
            if (transportProfile) {
                const next = Object.assign({}, temporaryProfiles)
                next[newUid] = transportProfile
                temporaryProfiles = next
            }
            tabs.remove(i)
            tabs.insert(i, {initialUrl:targetUrl, label:"Nouvel onglet", favicon:"",
                isPinned:pinned, isMuted:muted, audible:false, tabUid:newUid})
            activeTab = selected
            saveTimer.restart()
            if (wasCurrent && replacementUrl === undefined) window.showStatus(qsTr("Accès temporaire révoqué. Rouvrez la page pour continuer."))
            return newUid
        }
        return ""
    }
    function cancelPermission() { permissionCoordinator.cancel() }
    function resolvePermission(allow, requestId) {
        return permissionCoordinator.resolve(allow, currentView, currentProfile,
            requestId === undefined ? permissionCoordinator.pendingRequestId : requestId)
    }
    function askPermission(permission, view) {
        if (permission.permissionType === WebEnginePermission.PermissionType.Geolocation && !browserStore.geolocationAvailable)
            window.showStatus(qsTr("La localisation est désactivée dans cette version de CYBOU."))
        permissionCoordinator.submit(permission, view, view.profile, view === currentView,
            controlState.shouldDeny(permission, view ? view.url : "", view ? view.tabUid : ""))
    }
    function clearBrowsingData(cookies, cache, history, permissions, session) {
        cancelPermission()
        if (cookies || session) {
            tabs.clear(); closedTabs = []; addTab("")
        }
        if (history && !privateMode) browserStore.clearHistory()
        if (session && !privateMode) browserStore.saveSession([])
        if (permissions) {
            permissionCoordinator.revokeAll()
            const decisions = activeProfile.listAllPermissions()
            for (const decision of decisions) decision.reset()
            controlState.clearPermissionPolicies()
        }
        if (cookies) browserStore.clearCookies(activeProfile)
        if (cache) browserStore.clearCache(activeProfile)
        settingsPanel.statusText = "Effacement demandé pour les catégories sélectionnées. Les cookies sont supprimés en arrière-plan."
        settingsPanel.refreshPermissions()
    }
    // CYB-SBX-002 storage isolation (driven by cybou_tests): open the storage fixture in this window's profile, run
    // an async probe in the page, and expose its JSON result for the test to poll.
    property string storageProbeResult: ""
    property int storageProbeRunId: 0
    // Callbacks of view.runJavaScript still in flight: Qt WebEngine 6.140 crashes the process when a view is destroyed
    // before its runJavaScript callback returns (reproduced 2026-10-28), so the probe tab closes only at zero.
    property int storageProbeOutstanding: 0
    function storageProbeOpen(path) { addTab(smokeBaseUrl + (path || "/storage")) }
    function storageProbeRendererPid() { return currentView ? currentView.renderProcessPid : 0 }
    function storageProbeClose() { if (currentView && currentView.url.toString().startsWith(smokeBaseUrl + "/")) closeTab(activeTab) }
    function storageProbeReady() { return currentView && currentView.title.endsWith("-ready") && !currentView.loading }
    function storageProbeUrl() { return currentView ? currentView.url.toString() : "" }
    // Each run is numbered and its result carries the number: a late answer from an earlier run is ignored.
    function storageProbeRun(mode, marker) {
        storageProbeResult = ""
        const id = ++storageProbeRunId
        currentView.runJavaScript("window.cybouProbeResult=null;window.cybouProbe(" + JSON.stringify(mode) + "," + JSON.stringify(marker)
                                  + ").then(r=>window.cybouProbeResult=JSON.stringify({id:" + id + ",r}),e=>window.cybouProbeResult=JSON.stringify({id:" + id + ",r:{error:String(e)}}))")
    }
    function storageProbePoll() {
        const id = storageProbeRunId
        ++storageProbeOutstanding
        currentView.runJavaScript("window.cybouProbeResult", function(value) {
            --storageProbeOutstanding
            if (!value) return
            const answer = JSON.parse(value)
            if (answer.id === id && id === storageProbeRunId) storageProbeResult = JSON.stringify(answer.r)
        })
        return storageProbeResult
    }
    function runPrivateSmokeTest() {
        return privateMode && privateProfile && privateProfile.offTheRecord && persistentProfile === null && tabs.count === 1
    }
    function prepareDragSmokeTest() {
        controlBar.dismiss()
        library.close()
        tabs.clear()
        for (let i = 0; i < 3; ++i) addTab(smokeBaseUrl + "/drag" + i)
        activeTab = 0
    }
    // Centre of the first tab and just past the centre of the third, re-read by the test until the layout is stable.
    function dragSmokePoints() {
        tabStrip.forceLayout()
        const first = tabStrip.itemAtIndex(0)
        const last = tabStrip.itemAtIndex(2)
        const start = first.mapToItem(null, first.width / 2, first.height / 2)
        const end = last.mapToItem(null, last.width / 2 + 10, last.height / 2)
        return { x1: start.x, y1: start.y, x2: end.x, y2: end.y }
    }
    // Popups (and their modal dimmers) live in the window overlay and take presses while visible, including during
    // their exit transition; the drag test waits for an empty overlay and reports what was there if it still fails.
    function dragSmokeOverlay() {
        const shown = []
        for (const child of Overlay.overlay.children) if (child.visible) shown.push(child.objectName || child.toString())
        return shown
    }
    function dragSmokeHit(x, y) {
        let item = window.contentItem, path = []
        while (item) { path.push(item.objectName || item.toString()); const p = window.contentItem.mapToItem(item, x, y); item = item.childAt(p.x, p.y) }
        return path.slice(-3).join(" > ")
    }
    function verifyDragSmokeTest() { return activeTab === 2 && currentView.url.toString() === smokeBaseUrl + "/drag0" }
    function prepareSuggestionsSmokeTest() {
        const url = smokeBaseUrl + "/cybou-suggest"
        if (browserStore.isBookmarked(url)) browserStore.toggleBookmark(url, "")
        browserStore.toggleBookmark(url, "CYBOU suggestion test")
        address.forceActiveFocus(); address.selectAll()
    }
    function verifySuggestionsSmokeTest() {
        const url = smokeBaseUrl + "/cybou-suggest"
        const valid = currentView.url.toString() === url && !suggestionsPopup.visible
        browserStore.toggleBookmark(url, "")
        return valid
    }
    function prepareFeaturePreview() {
        window.width = 1360; window.height = 900
        tabs.clear()
        addTab("about:blank", {pinned: true})
        addTab("about:blank", {muted: true})
        tabs.setProperty(1, "label", "Espace de travail")
        addTab("about:blank")
        const url = smokeBaseUrl + "/preview"
        if (!browserStore.isBookmarked(url)) browserStore.toggleBookmark(url, "CYBOU — Espace personnel")
    }
    function showSuggestionsPreview() {
        address.forceActiveFocus(); address.text = "CYBOU"; updateSuggestions()
    }
    function showCloudLauncherPreview() {
        if (newTabPage) newTabPage.openCloudLauncher()
    }
    function cleanFeaturePreview() {
        const url = smokeBaseUrl + "/preview"
        if (browserStore.isBookmarked(url)) browserStore.toggleBookmark(url, "")
        tabs.clear(); addTab(""); saveSession()
    }
    function testViewAt(index) { return views.itemAt(index) }
    readonly property var transportTestProfile: trackerTestMode ? trackerHarness.trackedProfile : transportHarness.trackedProfile
    function beginTransportTest() { if (trackerTestMode) trackerHarness.begin(); else transportHarness.begin() }
    function pollTransportTest() { return trackerTestMode ? trackerHarness.step() : transportHarness.step() }
    function takeTransportScreenshot() { return trackerTestMode ? trackerHarness.takeScreenshot() : transportHarness.takeScreenshot() }
    function takeTransportInput() { return trackerTestMode ? trackerHarness.takeInput() : transportHarness.takeInput() }
    function testHttpButtonPoint() {
        const p = httpButton.mapToItem(null, httpButton.width / 2, httpButton.height / 2)
        return {kind:"click", x:p.x, y:p.y}
    }
    function prepareSurfaceTest(section) {
        library.close(); settingsPanel.close()
        if (section === "settings") settingsPanel.open()
        else if (section.length) window.showLibrary(section)
    }
    function surfaceTestState() {
        const panel = settingsPanel.visible ? settingsPanel : library
        const page = window.contentItem.mapToItem(Overlay.overlay, 0, 0)
        return {visible:panel.visible, y:panel.y, height:panel.height, pageY:page.y,
            pageHeight:window.contentItem.height, modal:panel.modal, dim:panel.dim}
    }
    function surfaceHintPoint() {
        const p = downloadsButton.mapToItem(null, downloadsButton.width / 2, downloadsButton.height / 2)
        return {x:p.x,y:p.y}
    }
    function surfaceHintVisible() { return downloadsButton.hintVisible }
    // Point (window coordinates) in the middle of the index-th link of the external-link fixture page, or null until it is loaded.
    function surfaceLinkPoint(index) {
        const view = currentView
        if (!view || view.loading || view.title !== "External link fixture") return null
        const p = view.mapToItem(null, 150, 85 + index * 90)
        return {x: p.x, y: p.y}
    }
    function surfaceRenderPid() { return currentView ? currentView.renderProcessPid : 0 }
    function surfaceFocusPage(onPage) { if (onPage) { address.focus = false; if (currentView) currentView.forceActiveFocus() } else address.forceActiveFocus() }
    function surfaceActiveTab() { return activeTab }
    function surfaceAddressFocused() { return address.activeFocus }
    function surfaceTabCount() { return tabs.count }
    function surfaceTabsLoaded(title) {
        let n = 0
        for (let i = 0; i < tabs.count; ++i) { const v = views.itemAt(i); if (v && !v.loading && v.title === title) ++n }
        return n
    }
    function surfaceCloseExtraTabs() { while (tabs.count > 1) closeTab(tabs.count - 1) }
    function surfaceDismissStatus() { statusPopup.close() }
    function surfaceStatusText() { return statusPopup.visible ? statusLabel.text : "" }
    function surfaceStatusIsPlain() { return statusLabel.textFormat === Text.PlainText }
    function testTitleDragPoint() {
        const p = emptyTabDrag.mapToItem(null, emptyTabDrag.width / 2, emptyTabDrag.height / 2)
        return {x:p.x, y:p.y, width:emptyTabDrag.width}
    }
    TrackerHarness { id: trackerHarness; browserWindow: window; controller: controlState; bar: controlBar }
    TransportHarness { id: transportHarness; browserWindow: window; controller: controlState; bar: controlBar }
    function testTabCount() { return tabs.count }
    function testAddressMatches() { return currentView && address.text === (currentView.url.toString() === "about:blank" ? "" : currentView.url.toString()) }
    function testOverlayVisibility() {
        settingsPanel.open(); const settingsHidden = !controlBar.interactionsEnabled && !controlBar.hasPopup; settingsPanel.close()
        library.open(); const libraryHidden = !controlBar.interactionsEnabled && !controlBar.hasPopup; library.close()
        window.showFullScreen(); const fullscreenHidden = !controlBar.interactionsEnabled && !controlBar.hasPopup; window.showNormal()
        return settingsHidden && libraryHidden && fullscreenHidden
    }
    function testOmnibox() {
        return { navWidth:navRow.width, fixedWidth:6 * 30 + 9 + 9 * navRow.spacing,
            addressWidth:addressContainer.width, editing:address.activeFocus,
            visibleKeys:controlBar.visibleKeys, overflowKeys:controlBar.hiddenKeys,
            overflowVisible:controlBar.overflowVisible, controlWidth:controlBar.effectiveWidth,
            controlRight:controlBar.x + controlBar.width, separatorX:panelSeparator.x, separatorRight:panelSeparator.x + panelSeparator.width,
            downloadX:downloadsButton.x, menuX:menuButton.x, currentTabId:currentView ? currentView.tabUid : "",
            displayHost:address.displayHost, rawUrl:currentView ? currentView.url.toString() : "",
            input:address.text, selectedText:address.selectedText }
    }
    function testAddressPoint() { const p = address.mapToItem(null, address.width / 2, address.height / 2); return {kind:"click",x:p.x,y:p.y} }
    function testDraft(value) { address.text = value }
    function testLocalTextFile() { for (const item of downloadItems) if (item.downloadFileName.endsWith(".txt")) return browserStore.fileUrl(downloadPath(item)); return "" }
    function testBrowse() { address.focus = false; if (currentView) currentView.forceActiveFocus(); refreshAddress() }
    // --media-test reuses the Control Bar test driver with the camera permission harness.
    readonly property var activeHarness: (typeof mediaTestMode !== "undefined" && mediaTestMode) ? mediaHarness : controlHarness
    function beginControlBarTest() { activeHarness.begin() }
    function pollControlBarTest() { return activeHarness.step() }
    function takeControlBarScreenshot() { return activeHarness.takeScreenshot() }
    function takeControlBarInput() { return activeHarness.takeInput() }
    ControlBarHarness { id: controlHarness; browserWindow: window; controller: controlState; bar: controlBar; coordinator: permissionCoordinator }
    MediaPermissionHarness { id: mediaHarness; browserWindow: window; controller: controlState; coordinator: permissionCoordinator }
    property int smokeStage: 0
    property int smokePdfResult: 0
    property bool smokeLocalLoaded: false
    property bool smokeCookiePresent: false
    property bool smokeCookieRead: false
    function pollSmokeTest() {
        if (smokeStage === 0 && currentView.title === "CYBOU smoke page" && !currentView.loading) {
            const decision = activeProfile.queryPermission(smokeBaseUrl, WebEnginePermission.PermissionType.Notifications)
            decision.reset()
            currentView.findText("needle")
            smokeStage = 1
            return 0
        }
        if (smokeStage === 1 && findResult === "1 / 1") {
            currentView.url = smokeBaseUrl + "/download"
            smokeStage = 2
            return 0
        }
        if (smokeStage === 2 && downloadItems.length && downloadItems[0].isFinished) {
            if (downloadItems[0].state !== WebEngineDownloadRequest.DownloadCompleted) return -1
            reviewDownloadedFile(downloadItems[0])
            smokeStage = 21
            return 0
        }
        if (smokeStage === 21) {
            if (!downloadOpenDialog.visible || !downloadOpenDialog.allowed || !pendingOpenPath.endsWith(".txt") || pendingOpenOrigin !== browserStore.origin(smokeBaseUrl)) return -1
            downloadOpenDialog.reject()
            smokeStage = 22
            return 0
        }
        if (smokeStage === 22) {
            if (downloadOpenDialog.visible || pendingOpenPath.length || pendingOpenOrigin.length) return 0
            console.log("DOWNLOAD_REVIEW_TEST PASS: real completed download, source, cancellation and state cleanup")
            library.close()
            currentView.runJavaScript("Notification.requestPermission()")
            smokeStage = 3
            return 0
        }
        if (smokeStage === 3 && pendingPermission) {
            if (pendingPermission.permissionType !== WebEnginePermission.PermissionType.Notifications) return -1
            if (!controlState.capability("notifications").pending || controlState.capability("notifications").requester !== browserStore.origin(smokeBaseUrl)) return -1
            resolvePermission(true)
            if (!currentView.hasPermissionLease || !currentView.grantedKeys.includes("notifications")) return -1
            smokeStage = 4
            currentView.runJavaScript("document.cookie", function(value) { smokeCookiePresent = value.includes("cybou_smoke=present"); smokeCookieRead = true })
            return 0
        }
        if (smokeStage === 4 && smokeCookieRead) {
            if (!smokeCookiePresent) return -1
            clearBrowsingData(true, true, true, true, true)
            if (browserStore.history.length !== 0 || activeProfile.listAllPermissions().length !== 0 || tabs.count !== 1) return -1
            smokeStage = 5
            return 0
        }
        if (smokeStage === 5 && !browserStore.cacheBusy) {
            smokeCookieRead = false
            currentView.url = smokeBaseUrl + "/check"
            smokeStage = 6
            return 0
        }
        if (smokeStage === 6 && currentView.title === "CYBOU smoke page" && !currentView.loading) {
            smokeStage = 7
            currentView.runJavaScript("document.cookie", function(value) { smokeCookiePresent = value.includes("cybou_smoke="); smokeCookieRead = true })
            return 0
        }
        if (smokeStage === 7 && smokeCookieRead) {
            if (smokeCookiePresent) return -1
            pdfTarget = currentView; pdfBusy = true
            currentView.printToPdf(smokeDownloadPath + "/cybou-smoke.pdf")
            smokeStage = 8
            return 0
        }
        if (smokeStage === 8 && smokePdfResult !== 0) {
            if (smokePdfResult !== 1) return -1
            openLocalFile(browserStore.fileUrl(smokeDownloadPath + "/" + downloadItems[0].downloadFileName))
            smokeStage = 9
            return 0
        }
        if (smokeStage === 9 && smokeLocalLoaded) {
            currentView.url = smokeBaseUrl + "/slow-download"
            smokeStage = 10
            return 0
        }
        if (smokeStage === 10 && downloadItems[0].downloadFileName.startsWith("cybou-slow") && downloadItems[0].receivedBytes > 0 && !downloadItems[0].isFinished) {
            downloadItems[0].pause(); smokeStage = 11
            return 0
        }
        if (smokeStage === 11 && downloadItems[0].isPaused) {
            window.close()
            if (!closeWorkDialog.visible) return -1
            closeWorkDialog.reject()
            downloadItems[0].resume(); smokeStage = 12
            return 0
        }
        if (smokeStage === 12 && downloadItems[0].isFinished) {
            if (downloadItems[0].state !== WebEngineDownloadRequest.DownloadCompleted || downloadItems[0].receivedBytes !== 4194304) return -1
            reviewDownloadedFile(downloadItems[0])
            smokeStage = 23
            return 0
        }
        if (smokeStage === 23) {
            if (!downloadOpenDialog.visible || downloadOpenDialog.allowed || !pendingOpenPath.endsWith(".bin")) return -1
            console.log("DOWNLOAD_BLOCKED_TYPE_TEST PASS")
            downloadOpenDialog.reject()
            smokeStage = 24
            return 0
        }
        if (smokeStage === 24) {
            if (downloadOpenDialog.visible || pendingOpenPath.length) return 0
            library.close()
            currentView.url = "about:blank"; saveSession()
            settingsPanel.close()
            statusPopup.close()
            return 2
        }
        return 0
    }

    function runSmokeTest() {
        try {
            tabs.clear(); addTab("")
            addTab(smokeBaseUrl + "/one")
            addTab(smokeBaseUrl + "/two")
            if (tabs.count !== 3 || activeTab !== 2) throw new Error("create tabs")
            moveTab(2, 1)
            if (activeTab !== 1 || currentView.url.toString() !== smokeBaseUrl + "/two") throw new Error("move tab identity")
            closeTab(1)
            if (tabs.count !== 2) throw new Error("close tab")
            reopenTab()
            if (tabs.count !== 3 || currentView.url.toString() !== smokeBaseUrl + "/two") throw new Error("reopen tab")
            togglePin(activeTab)
            if (activeTab !== 0 || pinnedCount() !== 1 || currentView.url.toString() !== smokeBaseUrl + "/two") throw new Error("pin identity")
            toggleMute(0)
            if (!views.itemAt(0).audioMuted) throw new Error("mute tab")
            addTab(smokeBaseUrl + "/normal")
            moveTab(activeTab, 0)
            if (activeTab !== 1 || !tabs.get(0).isPinned) throw new Error("pinned group boundary")
            closeTab(0); reopenTab()
            if (activeTab !== 0 || !tabs.get(0).isPinned || !currentView.audioMuted) throw new Error("reopen pinned muted tab")
            saveSession()
            const session = browserStore.session()
            tabs.clear()
            for (const entry of session) restoreTab(entry)
            if (!tabs.get(0).isPinned || !views.itemAt(0).audioMuted) throw new Error("restore tab flags")
            restoreTab("about:blank")
            if (currentView.url.toString() !== "about:blank") throw new Error("legacy session format")
            const suggestUrl = smokeBaseUrl + "/cybou-suggest-bookmark"
            browserStore.clearHistory()
            browserStore.toggleBookmark(suggestUrl, "CYBOU suggest")
            browserStore.recordVisit(suggestUrl, "CYBOU suggest")
            browserStore.recordVisit(smokeBaseUrl + "/cybou-suggest-history", "CYBOU suggest")
            if (browserStore.suggestions("cybou-suggest", true).length !== 2 || browserStore.suggestions("cybou-suggest", false).length !== 1) throw new Error("private suggestions or deduplication")
            browserStore.toggleBookmark(suggestUrl, "")
            browserStore.toggleBookmark("https://example.org/test", "Test")
            if (!browserStore.isBookmarked("https://example.org/test")) throw new Error("bookmark add")
            browserStore.toggleBookmark("https://example.org/test", "Test")
            if (browserStore.isBookmarked("https://example.org/test")) throw new Error("bookmark remove")
            browserStore.recordVisit("https://example.org/test", "Test")
            if (!browserStore.history.length) throw new Error("history")
            saveSession()
            const count = tabs.count
            browserStore.newPrivateWindow()
            browserStore.newPrivateWindow()
            if (privateMode || tabs.count !== count) throw new Error("private window changed ordinary tabs")
            const engine = browserStore.searchEngine
            browserStore.searchEngine = "Brave"
            if (browserStore.searchUrl("a & b") !== "https://search.brave.com/search?q=a%20%26%20b") throw new Error("search provider")
            browserStore.searchEngine = engine
            const restore = browserStore.restoreSession
            browserStore.restoreSession = !restore
            if (browserStore.restoreSession === restore) throw new Error("startup setting")
            browserStore.restoreSession = restore
            const block = browserStore.blockThirdPartyCookies
            browserStore.blockThirdPartyCookies = !block
            if (browserStore.blockThirdPartyCookies === block) throw new Error("cookie filter setting")
            browserStore.blockThirdPartyCookies = block
            tabs.clear(); addTab(""); saveSession()
            currentView.url = smokeBaseUrl + "/page"
            return true
        } catch (error) { console.error("SMOKE:", error); return false }
    }

    function addTab(url, options) {
        const pinned = options ? !!options.pinned : false
        const index = pinned ? pinnedCount() : tabs.count
        tabs.insert(index, { initialUrl: url || "about:blank", label: "Nouvel onglet", favicon: "", isPinned: pinned, isMuted: options ? !!options.muted : false, audible: false, tabUid: "tab-" + (++nextTabUid) })
        activeTab = index
        tabStrip.positionViewAtIndex(activeTab, ListView.Contain)
        saveTimer.restart()
    }
    // Ctrl+1..8 select that tab (the last one when there are fewer), Ctrl+9 always the last.
    function selectTabByNumber(n) { activeTab = n >= 9 ? tabs.count - 1 : Math.min(n - 1, tabs.count - 1) }
    function cycleTab(step) { activeTab = (activeTab + step + tabs.count) % tabs.count }
    function closeTab(index) {
        const view = views.itemAt(index)
        if (pdfBusy && view === pdfTarget) { showStatus("L’export PDF est en cours."); return }
        if (view === pendingPermissionView) cancelPermission()
        if (!privateMode && view) closedTabs = closedTabs.concat([{url: view.url.toString(), pinned: tabs.get(index).isPinned, muted: tabs.get(index).isMuted}])
        const next = activeTab > index ? activeTab - 1 : Math.min(activeTab, tabs.count - 2)
        tabs.remove(index)
        activeTab = Math.max(0, next)
        if (!tabs.count) addTab("")
        saveTimer.restart()
    }
    function reopenTab() {
        if (!closedTabs.length) return
        const url = closedTabs[closedTabs.length - 1]
        closedTabs = closedTabs.slice(0, -1)
        restoreTab(url)
    }
    function moveTab(from, to, allowGroupChange) {
        if (to < 0 || to >= tabs.count) return
        if (!allowGroupChange) to = tabs.get(from).isPinned ? Math.min(to, pinnedCount() - 1) : Math.max(to, pinnedCount())
        if (from === to) return
        const selected = activeTab
        tabs.move(from, to, 1)
        if (selected === from) activeTab = to
        else if (from < selected && to >= selected) activeTab--
        else if (from > selected && to <= selected) activeTab++
        saveTimer.restart()
    }
    function saveSession() {
        if (privateMode || restoring) return
        let urls = []
        for (let i = 0; i < views.count; ++i) {
            const view = views.itemAt(i)
            if (view) urls.push({url: view.url.toString(), pinned: tabs.get(i).isPinned, muted: tabs.get(i).isMuted})
        }
        browserStore.saveSession(urls)
    }
    function navigate(value) {
        suggestionsPopup.close()
        const input = value.trim()
        if (!input || !currentView) return
        let target
        if (/^https?:\/\//i.test(input)) target = input
        else if (!/\s/.test(input) && (input.includes(".") || /^localhost(:\d+)?(\/|$)/.test(input))) target = "https://" + input
        else target = browserStore.searchUrl(input)
        if (browserStore.shouldUpgradeToHttps(target)) {
            currentView.upgradedFromHttpUrl = target
            currentView.upgradeExpectedUrl = browserStore.upgradeToHttps(target).toString()
            currentView.url = currentView.upgradeExpectedUrl
        } else {
            currentView.upgradedFromHttpUrl = ""
            currentView.upgradeExpectedUrl = ""
            currentView.url = target
        }
        currentView.forceActiveFocus()
    }
    function refreshAddress() {
        if (!address.activeFocus) address.text = currentView && currentView.url.toString() !== "about:blank" ? currentView.url.toString() : ""
        bookmarked = currentView ? browserStore.isBookmarked(currentView.url.toString()) : false
    }
    function bookmark() {
        if (currentView) browserStore.toggleBookmark(currentView.url.toString(), currentView.title)
    }
    function showLibrary(section) {
        if (section === "Favoris" || section === "bookmarks") librarySection = "bookmarks"
        else if (section === "Historique" || section === "history") librarySection = "history"
        else if (section === "Téléchargements" || section === "downloads") librarySection = "downloads"
        else librarySection = section || "bookmarks"
        library.open()
    }
    function maximizeToggle() { visibility === Window.Maximized ? showNormal() : showMaximized() }
    function handleDownload(download) {
        if (currentView && currentView.restrictedMode && !currentView.restrictedModeOverridden) {
            download.cancel()
            window.showStatus(qsTr("Téléchargement bloqué : site en mode restreint."))
            return
        }
        download.downloadDirectory = smokeMode ? smokeDownloadPath : browserStore.downloadsPath
        downloadItems = [download].concat(downloadItems)
        showLibrary("downloads")
        if (browserStore.askDownloadLocation && !smokeMode) {
            downloadQueue = downloadQueue.concat([download]); presentNextDownload()
        } else download.accept()
    }
    Timer { id: saveTimer; interval: 700; onTriggered: saveSession() }
    onActiveTabChanged: { policyBanner.close(); refreshAddress(); findVisible = false; if (controlBar) controlBar.dismiss() }
    onCurrentViewChanged: { if (suggestionsPopup) suggestionsPopup.close(); if (controlBar) controlBar.dismiss(); if (address) { address.focus = false; refreshAddress() } }
    onClosing: function(event) {
        if (!closingApproved && (activeDownloads() || pdfBusy)) {
            event.accepted = false; closeWorkDialog.open(); return
        }
        cancelPermission(); saveSession()
    }
    Connections { target: browserStore; function onChanged() { window.refreshAddress() } }
    property var persistentProfile: null
    property var privateProfile: null
    Component {
        id: normalPrototype
        WebEngineProfilePrototype {
            storageName: smokeMode ? "CybouSmokeTest" : "Cybou"
            persistentCookiesPolicy: WebEngineProfile.AllowPersistentCookies
            persistentPermissionsPolicy: WebEngineProfile.AskEveryTime
        }
    }
    Component { id: privatePrototype; WebEngineProfilePrototype { persistentPermissionsPolicy: WebEngineProfile.AskEveryTime } }
    Connections { target: persistentProfile; function onDownloadRequested(download) { window.handleDownload(download) } }
    Connections { target: privateProfile; function onDownloadRequested(download) { window.handleDownload(download) } }
    ListModel { id: tabs }
    Component.onCompleted: {
        if (privateMode) privateProfile = privatePrototype.createObject(window).instance()
        else persistentProfile = normalPrototype.createObject(window).instance()
        browserStore.configureProfile(activeProfile)
        const saved = !privateMode && !smokeMode && browserStore.restoreSession ? browserStore.session() : []
        for (let i = 0; i < saved.length; ++i) restoreTab(saved[i])
        if (!privateMode && !smokeMode && browserStore.takeStrictWebRtcNotice())
            Qt.callLater(function() { showStatus(qsTr("WebRTC strict est activé : les appels vidéo qui exigent de l’UDP direct peuvent échouer. Réglages › Sécurité pour le désactiver.")) })
        if (initialUrlArgument.length > 0) addTab(initialUrlArgument)
        else if (!tabs.count) addTab("")
        restoring = false
        if (initialLibrarySection.length > 0) showLibrary(initialLibrarySection)
        if (initialSettingsOpen) settingsPanel.open()
        if (initialMenuOpen) menu.popup(menuButton, 0, menuButton.height)
        if (initialCloudLauncherOpen) Qt.callLater(function() { showCloudLauncherPreview() })
    }
    Shortcut { sequence: "Ctrl+L"; onActivated: { address.forceActiveFocus(); Qt.callLater(function() { address.selectAll() }) } }
    Shortcut { sequence: "Ctrl+T"; onActivated: addTab("") }
    Shortcut { sequence: "Ctrl+W"; onActivated: closeTab(activeTab) }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: reopenTab() }
    Shortcut { sequence: "Ctrl+Tab"; onActivated: cycleTab(1) }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: cycleTab(-1) }
    Shortcut { sequence: "Ctrl+R"; onActivated: if (currentView) currentView.reload() }
    Shortcut { sequence: "F5"; onActivated: if (currentView) currentView.reload() }
    Shortcut { sequence: "Ctrl+Shift+R"; onActivated: if (currentView) currentView.reloadAndBypassCache() }
    Shortcut { sequence: "Ctrl+F5"; onActivated: if (currentView) currentView.reloadAndBypassCache() }
    Shortcut { sequence: "Alt+D"; onActivated: { address.forceActiveFocus(); Qt.callLater(function() { address.selectAll() }) } }
    Shortcut { sequence: "F6"; onActivated: { address.forceActiveFocus(); Qt.callLater(function() { address.selectAll() }) } }
    Shortcut { sequence: "Ctrl+PgDown"; onActivated: cycleTab(1) }
    Shortcut { sequence: "Ctrl+PgUp"; onActivated: cycleTab(-1) }
    // Ctrl+1..8 select that tab, Ctrl+9 the last one (browser convention).
    Shortcut { sequence: "Ctrl+1"; onActivated: selectTabByNumber(1) }
    Shortcut { sequence: "Ctrl+2"; onActivated: selectTabByNumber(2) }
    Shortcut { sequence: "Ctrl+3"; onActivated: selectTabByNumber(3) }
    Shortcut { sequence: "Ctrl+4"; onActivated: selectTabByNumber(4) }
    Shortcut { sequence: "Ctrl+5"; onActivated: selectTabByNumber(5) }
    Shortcut { sequence: "Ctrl+6"; onActivated: selectTabByNumber(6) }
    Shortcut { sequence: "Ctrl+7"; onActivated: selectTabByNumber(7) }
    Shortcut { sequence: "Ctrl+8"; onActivated: selectTabByNumber(8) }
    Shortcut { sequence: "Ctrl+9"; onActivated: selectTabByNumber(9) }
    Shortcut { sequence: "Ctrl+D"; onActivated: bookmark() }
    Shortcut { sequence: "Ctrl+H"; onActivated: showLibrary("history") }
    Shortcut { sequence: "Ctrl+J"; onActivated: showLibrary("downloads") }
    Shortcut { sequence: "Ctrl+F"; onActivated: { findVisible = true; findBar.focusInput() } }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: browserStore.newPrivateWindow() }
    Shortcut { sequence: "Ctrl+,"; onActivated: settingsPanel.open() }
    Shortcut { sequence: "Ctrl+O"; onActivated: openFileDialog.open() }
    Shortcut { sequence: "Ctrl+P"; onActivated: exportPdf() }
    Shortcut { sequence: "Ctrl+M"; onActivated: toggleMute(activeTab) }
    Shortcut { sequence: "Alt+Left"; onActivated: if (currentView) currentView.goBack() }
    Shortcut { sequence: "Alt+Right"; onActivated: if (currentView) currentView.goForward() }
    Shortcut { sequence: "F11"; onActivated: visibility === Window.FullScreen ? showNormal() : showFullScreen() }
    Shortcut { sequence: "Ctrl++"; onActivated: if (currentView) currentView.zoomFactor = Math.min(3, Math.round((currentView.zoomFactor + 0.1) * 10) / 10) }
    Shortcut { sequence: "Ctrl+="; onActivated: if (currentView) currentView.zoomFactor = Math.min(3, Math.round((currentView.zoomFactor + 0.1) * 10) / 10) }
    Shortcut { sequence: "Ctrl+-"; onActivated: if (currentView) currentView.zoomFactor = Math.max(0.3, Math.round((currentView.zoomFactor - 0.1) * 10) / 10) }
    Shortcut { sequence: "Ctrl+0"; onActivated: if (currentView) currentView.zoomFactor = 1 }
    Shortcut { sequence: "Escape"; enabled: !address.activeFocus && !controlBar.hasPopup; onActivated: { findVisible = false; if (currentView) { currentView.findText(""); currentView.stop() } } }

    header: ColumnLayout {
        spacing: 0
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: chromeRowHeight; Layout.minimumHeight: chromeRowHeight; Layout.maximumHeight: chromeRowHeight
            color: privateMode ? "#060F1E" : "#081325"
            WindowDragArea { anchors.fill: parent }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8
                spacing: 4
                Image { source: "qrc:/assets/cybou.png"; sourceSize.width: 32; sourceSize.height: 32; Layout.preferredWidth: 22; Layout.preferredHeight: 22 }
                ListView {
                    id: tabStrip
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.topMargin: 4; Layout.bottomMargin: 4
                    orientation: ListView.Horizontal
                    spacing: 3; clip: true
                    rightMargin: newTabBtn.implicitWidth + newTabButtonSpacing + 4
                    readonly property int newTabButtonSpacing: 12
                    model: tabs
                    NavButton {
                        id: newTabBtn
                        parent: tabStrip.contentItem
                        z: 3
                        text: "+"
                        hint: qsTr("Nouvel onglet · Ctrl+T")
                        implicitWidth: 28
                        implicitHeight: chromeControlHeight
                        x: Math.max(0, tabStrip.contentWidth + tabStrip.newTabButtonSpacing)
                        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                        onClicked: addTab("")
                    }
                    WindowDragArea {
                        id: emptyTabDrag
                        parent: tabStrip
                        // Flickable owns the unused strip too; provide an explicit title hit area to the right of tabs and new tab button.
                        x: Math.max(0, Math.min(tabStrip.width, (newTabBtn.x + newTabBtn.width + 4) - tabStrip.contentX))
                        width: Math.max(0, tabStrip.width - x); height: tabStrip.height
                        z: 2
                    }
                    delegate: Rectangle {
                        id: tabItem
                        required property int index
                        required property string label
                        required property string favicon
                        required property bool isPinned
                        required property bool isMuted
                        required property bool audible
                        width: isPinned ? (isMuted || audible ? 66 : 42) : Math.max(112, Math.min(200, (tabStrip.width - 34) / Math.min(tabs.count, 5)))
                        height: chromeControlHeight; radius: 7
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: activeTab === index ? "#E0183A68" : tabMouse.containsMouse ? "#701D3D6A" : "#200E223D" }
                            GradientStop { position: 1.0; color: activeTab === index ? "#E80E2444" : tabMouse.containsMouse ? "#50122846" : "#200E223D" }
                        }
                        color: "transparent"
                        border.width: 1
                        border.color: activeTab === index ? "#498BB8E0" : tabMouse.containsMouse ? "#30608BB8" : "transparent"
                        Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 140 } }
                        Rectangle {
                            opacity: activeTab === index ? 1.0 : tabMouse.containsMouse ? 0.4 : 0.0
                            Behavior on opacity { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 140 } }
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                            height: 1; color: "#35FFFFFF"; radius: parent.radius
                        }
                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent; hoverEnabled: true
                            preventStealing: true
                            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                            property real pressX: 0
                            onPressed: function(mouse) {
                                pressX = mapToItem(tabStrip, mouse.x, mouse.y).x
                                if (mouse.button === Qt.LeftButton) activeTab = tabItem.index
                            }
                            onPositionChanged: function(mouse) {
                                if (!(pressedButtons & Qt.LeftButton)) return
                                const position = mapToItem(tabStrip, mouse.x, mouse.y)
                                if (window.draggedTab === tabItem || Math.abs(position.x - pressX) > 8) {
                                    window.draggedTab = tabItem
                                    window.dragTabAt(tabItem, position.x)
                                }
                            }
                            onReleased: window.draggedTab = null
                            onCanceled: window.draggedTab = null
                            onClicked: function(mouse) {
                                if (mouse.button === Qt.MiddleButton) closeTab(tabItem.index)
                                else if (mouse.button === Qt.RightButton) tabContext.popup()
                                else activeTab = tabItem.index
                            }
                        }
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 9; anchors.rightMargin: 3; spacing: 6
                            Image { source: tabItem.favicon || "qrc:/assets/cybou.png"; Layout.preferredWidth: 15; Layout.preferredHeight: 15; sourceSize.width: 32; sourceSize.height: 32 }
                            Text { textFormat: Text.PlainText; visible: !tabItem.isPinned; text: tabItem.label === "Nouvel onglet" ? qsTr("Nouvel onglet") : tabItem.label; elide: Text.ElideRight; Layout.fillWidth: true; color: activeTab === tabItem.index ? "#FFFFFF" : "#8FA3BF"; font.bold: activeTab === tabItem.index; font.pixelSize: 12 }
                            NavButton {
                                visible: tabItem.isMuted || tabItem.audible; implicitWidth: 22; implicitHeight: 24
                                hint: tabItem.isMuted ? qsTr("Réactiver le son") : qsTr("Couper le son")
                                contentItem: Image { source: tabItem.isMuted ? "qrc:/assets/speaker-muted.svg" : "qrc:/assets/speaker.svg"; sourceSize.width: 16; sourceSize.height: 16; fillMode: Image.PreserveAspectFit }
                                onClicked: toggleMute(tabItem.index)
                            }
                            NavButton { visible: !tabItem.isPinned; text: "×"; implicitWidth: 22; implicitHeight: 24; hint: qsTr("Fermer · Ctrl+W"); onClicked: closeTab(tabItem.index) }
                        }
                        GlassToolTip { parent: tabItem; visible: tabMouse.containsMouse && !tabMouse.pressed; delay: 700; text: tabItem.label === "Nouvel onglet" ? qsTr("Nouvel onglet") : tabItem.label }
                        Menu {
                            id: tabContext
                            padding: 6
                            background: Rectangle {
                                implicitWidth: 200
                                radius: 10
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#E00E2648" }
                                    GradientStop { position: 1.0; color: "#F0061224" }
                                }
                                border.color: CybouTheme.borderHighlight
                                border.width: 1.5
                                layer.enabled: true
                                layer.effect: MultiEffect {
                                    shadowEnabled: true
                                    shadowColor: "#90000000"
                                    shadowBlur: 1.0
                                    shadowVerticalOffset: 10
                                }
                                Rectangle {
                                    anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                                    height: 1; color: CybouTheme.glassSheen; radius: parent.radius
                                }
                            }
                            GlassMenuItem { text: tabItem.isPinned ? qsTr("Désépingler l’onglet") : qsTr("Épingler l’onglet"); onTriggered: togglePin(tabItem.index) }
                            GlassMenuItem { text: tabItem.isMuted ? qsTr("Réactiver le son") : qsTr("Couper le son"); onTriggered: toggleMute(tabItem.index) }
                            GlassMenuItem { text: qsTr("Déplacer à gauche"); enabled: tabItem.index > 0; onTriggered: moveTab(tabItem.index, tabItem.index - 1) }
                            GlassMenuItem { text: qsTr("Déplacer à droite"); enabled: tabItem.index < tabs.count - 1; onTriggered: moveTab(tabItem.index, tabItem.index + 1) }
                            GlassMenuItem { text: qsTr("Dupliquer"); onTriggered: addTab(views.itemAt(tabItem.index).url.toString()) }
                            GlassMenuItem { text: qsTr("Fermer"); onTriggered: closeTab(tabItem.index) }
                            GlassMenuItem { text: qsTr("Rouvrir l’onglet fermé"); enabled: closedTabs.length > 0; onTriggered: reopenTab() }
                        }
                    }
                }
                Label { visible: privateMode; text: qsTr("PRIVÉ"); color: "#8FA3BF"; font.pixelSize: 10; padding: 8 }
                Item { Layout.preferredWidth: 18 }
                NavButton { text: "−"; hint: qsTr("Réduire"); implicitWidth: 38; implicitHeight: chromeRowHeight; onClicked: window.showMinimized() }
                NavButton { text: window.visibility === Window.Maximized ? "❐" : "□"; hint: qsTr("Agrandir / restaurer"); implicitWidth: 38; implicitHeight: chromeRowHeight; onClicked: window.maximizeToggle() }
                Button {
                    id: closeWindow
                    text: "×"; implicitWidth: 38; implicitHeight: chromeRowHeight; padding: 0; hoverEnabled: true
                    Accessible.name: qsTr("Fermer CYBOU")
                    background: Rectangle { color: closeWindow.hovered ? "#E11D2E" : "transparent" }
                    contentItem: Text { text: "×"; color: closeWindow.hovered ? "white" : "#8FA3BF"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: window.close()
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: chromeRowHeight; Layout.minimumHeight: chromeRowHeight; Layout.maximumHeight: chromeRowHeight
            color: "#0A182F"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#11243E" }
            RowLayout {
                id: navRow
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; anchors.topMargin: 4; anchors.bottomMargin: 4; spacing: 4
                NavButton { text: "←"; hint: qsTr("Retour"); enabled: currentView ? currentView.canGoBack : false; onClicked: currentView.goBack() }
                NavButton { text: "→"; hint: qsTr("Suivant"); enabled: currentView ? currentView.canGoForward : false; onClicked: currentView.goForward() }
                NavButton { text: currentView && currentView.loading ? "×" : "↻"; hint: qsTr("Actualiser / arrêter"); onClicked: currentView.loading ? currentView.stop() : currentView.reload() }
                NavButton { text: "⌂"; hint: qsTr("Accueil"); onClicked: currentView.url = "about:blank" }
                Rectangle {
                    id: addressContainer
                    Layout.fillWidth: true; Layout.minimumWidth: 120; implicitHeight: chromeControlHeight
                    radius: 8
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: address.activeFocus ? "#E80B1E38" : "#CC0F2748" }
                        GradientStop { position: 1.0; color: address.activeFocus ? "#F4061324" : "#DD091B32" }
                    }
                    border.width: address.activeFocus ? 1.5 : 1
                    border.color: address.activeFocus ? "#0088FF" : (currentView && currentView.restrictedMode && !currentView.restrictedModeOverridden ? "#D97706" : "#357DA5CC")
                    Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150 } }
                    Rectangle {
                        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                        height: 1; color: address.activeFocus ? "#4038BDF8" : "#25FFFFFF"; radius: parent.radius
                    }
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 5; anchors.rightMargin: 4; spacing: 3
                        NavButton {
                            id: httpButton
                            readonly property bool httpAction: window.transportState.httpPending || window.transportState.httpTemporary
                            property string pressedToken: ""
                            property bool pressedHttpAction: false
                            text: httpAction ? "HTTP" : (currentView && currentView.restrictedMode && !currentView.restrictedModeOverridden ? "⚠" : currentView && currentView.url.toString().startsWith("https:") ? "🔒" : "ⓘ")
                            implicitWidth: httpAction ? 36 : 24
                            hint: httpAction ? "" : (currentView && currentView.restrictedMode && !currentView.restrictedModeOverridden ? (qsTr("Site suspect en mode restreint : ") + currentView.restrictedReason) : currentView && currentView.url.toString().startsWith("https:") ? qsTr("Connexion chiffrée (HTTPS), sans garantie sur le site") : qsTr("Informations de connexion"))
                            Accessible.name: httpAction ? (window.transportState.httpTemporary ? qsTr("Révoquer l’accès HTTP temporaire") : qsTr("Autoriser cette adresse HTTP temporairement")) : hint
                            onPressed: { pressedToken = window.transportState.httpToken; pressedHttpAction = httpAction }
                            onClicked: {
                                if (!pressedHttpAction) {
                                    if (!httpAction) {
                                        if (currentView && currentView.restrictedMode && !currentView.restrictedModeOverridden) {
                                            policyBanner.showRestricted(currentView.restrictedReason, currentView.tabUid, currentView.navigationEpoch, currentView.url.toString())
                                        } else {
                                            connectionInfo.open()
                                        }
                                    }
                                    return
                                }
                                if (pressedToken !== window.transportState.httpToken) return
                                if (window.transportState.httpTemporary) window.replaceGrantedView(currentView)
                                else window.allowTemporaryHttp(pressedToken)
                            }
                            contentItem: Text {
                                textFormat: Text.PlainText
                                text: httpButton.text; font.pixelSize: httpButton.httpAction ? 10 : 17; font.bold: httpButton.httpAction
                                color: httpButton.httpAction ? "#FFD08A" : (currentView && currentView.restrictedMode && !currentView.restrictedModeOverridden ? "#F59E0B" : "#A0B3D0")
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                        }
                        AdaptiveAddress {
                            id: address
                            Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 0
                            displayHost: currentView ? browserStore.displayHost(currentView.url) : ""
                            onAccepted: window.acceptAddress()
                            onTextEdited: window.updateSuggestions()
                            onActiveFocusChanged: if (!activeFocus) { suggestionsPopup.close(); refreshAddress() }
                            Keys.onPressed: function(event) {
                                if (suggestionsPopup.visible && (event.key === Qt.Key_Down || event.key === Qt.Key_Up)) {
                                    suggestionIndex = event.key === Qt.Key_Down ? (suggestionIndex + 1) % suggestionRows.length : (suggestionIndex < 0 ? suggestionRows.length - 1 : (suggestionIndex + suggestionRows.length - 1) % suggestionRows.length)
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Escape) {
                                    suggestionsPopup.close(); focus = false; window.refreshAddress()
                                    if (currentView) currentView.forceActiveFocus()
                                    event.accepted = true
                                }
                            }
                        }
                        NavButton { text: bookmarked ? "★" : "☆"; hint: "Favori · Ctrl+D"; onClicked: bookmark() }
                    }
                }
                Item {
                    Layout.preferredWidth: 1; Layout.preferredHeight: 22
                    Layout.leftMargin: 8
                    Rectangle { anchors.fill: parent; color: "#456080" }
                }
                ControlBarInline {
                    id: controlBar
                    controller: controlState; popupHost: Overlay.overlay
                    pendingRequestId: permissionCoordinator.pendingRequestId
                    transportState: window.transportState
                    onAllowTrackersRequested: function(token) { window.allowTemporaryTrackers(token) }
                    onRevokeTrackersRequested: function(token) { window.revokeTemporaryTrackers(token) }
                    onAllowAdsRequested: function(token) { window.allowTemporaryAds(token) }
                    onRevokeAdsRequested: function(token) { window.revokeTemporaryAds(token) }
                    onAllowHttpRequested: function(token) { window.allowTemporaryHttp(token) }
                    onRevokeHttpRequested: function(token) { if (currentView && currentView.temporaryTransport && token === transportState.httpToken) window.replaceGrantedView(currentView) }
                    editing: false
                    widthBudget: effectiveWidth
                    Layout.preferredWidth: effectiveWidth; Layout.minimumWidth: effectiveWidth; Layout.maximumWidth: effectiveWidth
                    visible: true
                    interactionsEnabled: window.visibility !== Window.FullScreen && !library.visible && !settingsPanel.visible && !behaviorDialog.visible && !pageLoadErrorDialog.visible && !closeWorkDialog.visible && !downloadOpenDialog.visible
                    onPolicyRequested: function(key, mode) {
                        if (key === "error") { window.showStatus(qsTr("La modification n’a pas pu être confirmée.")); return }
                        if (currentView) {
                            const origin = browserStore.origin(currentView.url.toString())
                            const prevMode = key === "popups" ? controlState.popupMode(currentView.url) : (controlState.settingFor(currentView.url, key) ? 1 : 2)
                            if (controlState.setBehavior(key, mode)) {
                                if (key === "javascript" || key === "storage" || key === "popups") {
                                    policyBanner.contextEpoch = currentView.navigationEpoch; policyBanner.contextTab = currentView.tabUid; policyBanner.contextUrl = currentView.url.toString(); policyBanner.show(origin, key, mode, prevMode)
                                } else {
                                    window.showStatus(qsTr("Politique mise à jour pour %1").arg(origin))
                                }
                            } else {
                                window.showStatus(qsTr("La modification n’a pas pu être confirmée."))
                            }
                        }
                    }
                    onPermissionResetRequested: function(key, mode) {
                        const targetMode = mode === undefined ? 0 : mode
                        if (!currentView) return
                        if (["camera", "mic", "screen"].includes(key)) {
                            window.revokePermissionGracefully(currentView, key, targetMode)
                        } else {
                            if (controlState.setPermissionPolicy(key, targetMode))
                                window.replaceGrantedView(currentView, currentView.url.toString())
                        }
                    }
                    onDecisionRequested: function(allow, requestId) { window.resolvePermission(allow, requestId) }
                    onReloadRequested: if (currentView) { window.applyViewPolicy(currentView, currentView.url); currentView.reload(); controlBar.dismiss() }
                }
                Item {
                    id: panelSeparator
                    Layout.preferredWidth: 1; Layout.preferredHeight: 22
                    Layout.rightMargin: 8
                    Rectangle { anchors.centerIn: parent; width: 1; height: parent.height; color: "#456080" }
                }
                NavButton { id: downloadsButton; text: "↓"; hint: qsTr("Téléchargements · Ctrl+J"); onClicked: showLibrary("downloads") }
                NavButton { id: menuButton; text: "☰"; hint: qsTr("Menu"); onClicked: menu.popup(menuButton, 0, menuButton.height) }
                Menu {
                    id: menu
                    padding: 6
                    background: Rectangle {
                        implicitWidth: 260
                        radius: 12
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#E00E2648" }
                            GradientStop { position: 1.0; color: "#F0061224" }
                        }
                        border.color: CybouTheme.borderHighlight
                        border.width: 1.5
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowColor: "#90000000"
                            shadowBlur: 1.0
                            shadowVerticalOffset: 12
                        }
                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                            height: 1; color: CybouTheme.glassSheen; radius: parent.radius
                        }
                    }
                    GlassMenuItem { text: qsTr("Nouvel onglet                         Ctrl+T"); onTriggered: addTab("") }
                    GlassMenuItem { text: qsTr("Rouvrir l’onglet fermé          Ctrl+Shift+T"); enabled: closedTabs.length > 0; onTriggered: reopenTab() }
                    GlassMenuItem { text: qsTr("Nouvelle fenêtre privée     Ctrl+Shift+N"); onTriggered: browserStore.newPrivateWindow() }
                    GlassMenuSeparator {}
                    GlassMenuItem { text: qsTr("Favoris"); onTriggered: showLibrary("bookmarks") }
                    GlassMenuItem { text: qsTr("Historique                                Ctrl+H"); onTriggered: showLibrary("history") }
                    GlassMenuItem { text: qsTr("Téléchargements                      Ctrl+J"); onTriggered: showLibrary("downloads") }
                    GlassMenuItem { text: qsTr("Rechercher dans la page           Ctrl+F"); onTriggered: { findVisible = true; findBar.focusInput() } }
                    GlassMenuItem { text: qsTr("Ouvrir un fichier…                        Ctrl+O"); onTriggered: openFileDialog.open() }
                    GlassMenuItem { text: qsTr("Enregistrer en PDF…                   Ctrl+P"); enabled: currentView && !currentView.loading && currentView.url.toString() !== "about:blank" && !pdfBusy; onTriggered: exportPdf() }
                    GlassMenuSeparator {}
                    GlassMenuItem { text: qsTr("Zoom +                                     Ctrl++"); onTriggered: if (currentView) currentView.zoomFactor = Math.min(3, Math.round((currentView.zoomFactor + 0.1) * 10) / 10) }
                    GlassMenuItem { text: qsTr("Zoom −                                     Ctrl+-"); onTriggered: if (currentView) currentView.zoomFactor = Math.max(0.3, Math.round((currentView.zoomFactor - 0.1) * 10) / 10) }
                    GlassMenuItem { text: qsTr("Zoom 100 %                              Ctrl+0"); onTriggered: if (currentView) currentView.zoomFactor = 1 }
                    GlassMenuItem { text: qsTr("Plein écran                                  F11"); onTriggered: window.visibility === Window.FullScreen ? window.showNormal() : window.showFullScreen() }
                    GlassMenuSeparator {}
                    GlassMenuItem { text: qsTr("Paramètres                                     Ctrl+,"); onTriggered: settingsPanel.open() }
                    GlassMenuItem { text: qsTr("À propos de CYBOU"); onTriggered: about.open() }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 2; color: "#11243E"
            Rectangle { height: 2; width: parent.width * (currentView ? currentView.loadProgress / 100 : 0); color: "#0088FF"; visible: currentView ? currentView.loading : false }
        }
        PolicyBanner {
            id: policyBanner
            onReloadRequested: {
                if (currentView && currentView.tabUid === policyBanner.contextTab && currentView.navigationEpoch === policyBanner.contextEpoch && currentView.url.toString() === policyBanner.contextUrl) {
                    window.applyViewPolicy(currentView, currentView.url)
                    currentView.reload()
                }
            }
            onUndoRequested: function(key, prevMode) {
                if (key.length && currentView && currentView.tabUid === policyBanner.contextTab && currentView.navigationEpoch === policyBanner.contextEpoch && currentView.url.toString() === policyBanner.contextUrl) {
                    controlState.setBehavior(key, prevMode)
                    window.applyViewPolicy(currentView, currentView.url)
                }
            }
            onExitSiteRequested: {
                policyBanner.close()
                if (currentView) {
                    if (currentView.canGoBack) currentView.goBack()
                    else currentView.url = "about:blank"
                }
            }
            onEnableJsRequested: {
                policyBanner.close()
                if (currentView) {
                    currentView.restrictedModeOverridden = true
                    currentView.restrictedMode = false
                    currentView.reload()
                    window.showStatus(qsTr("JavaScript activé pour cette session."))
                }
            }
        }
        FindBar {
            id: findBar
            visible: window.findVisible
            resultText: window.findResult
            onTextChanged: if (currentView) currentView.findText(findBar.text)
            onFindNext: function(query) { if (currentView) currentView.findText(query) }
            onFindPrevious: function(query) { if (currentView) currentView.findText(query, WebEngineView.FindBackward) }
            onCloseRequested: {
                window.findVisible = false
                if (currentView) currentView.findText("")
            }
        }
    }
    Repeater {
        id: views; model: tabs
        delegate: BrowserTab {
            temporaryProfile: window.temporaryProfiles[tabUid] || null
            defaultProfile: window.activeProfile
            isCurrentTab: window.activeTab === index
            isPrivate: window.privateMode
            smokeMode: window.smokeActive
            store: browserStore
            coordinator: permissionCoordinator
            controller: controlState
            pendingPermissionView: window.pendingPermissionView

            onAskPermissionRequested: function(permission, view) { window.askPermission(permission, view) }
            onOfferHttpRequested: function(view, url) { window.offerHttp(view, url) }
            onEndTemporaryRequested: function(view, url) { window.endTemporaryNavigation(view, url) }
            onReleaseRequested: function(uid) { window.releaseTemporaryView(uid) }
            onDownloadRequested: function(download) { window.handleDownload(download) }
            onApplyPolicyRequested: function(view, url) { window.applyViewPolicy(view, url) }
            onNewTabRequested: function(url) { window.addTab(url) }
            onFullScreenToggled: function(on) { on ? window.showFullScreen() : window.showNormal() }
            onTabPropertyUpdate: function(idx, prop, val) { if (idx < tabs.count) tabs.setProperty(idx, prop, val) }
            onAddressRefresh: window.refreshAddress()
            onStatusNotice: function(msg) { window.showStatus(msg) }
            onErrorNotice: function(msg) { errorText.text = msg; pageLoadErrorDialog.open() }
            onDismissControlBar: if (controlBar) controlBar.dismiss()
            onClosePolicyBanner: if (policyBanner) policyBanner.close()
            onRestartSaveTimer: saveTimer.restart()
            onCancelPermissionRequested: window.cancelPermission()
            onRestrictedModeTriggered: function(view, reason) { if (view === currentView) policyBanner.showRestricted(reason, view.tabUid, view.navigationEpoch, view.url.toString()) }
            onFindResultUpdated: function(res) { window.findResult = res }
            onLocalLoaded: window.smokeLocalLoaded = true
            onPdfFinished: function(filePath, success) {
                window.pdfBusy = false; window.pdfTarget = null
                window.showStatus(success ? "PDF enregistré : " + filePath : "Échec de l’enregistrement PDF.")
                window.smokePdfResult = success ? 1 : -1
            }
            onContextMenuOpen: function(targetView, link, text, editable, x, y) {
                pageContextMenu.linkUrl = link
                pageContextMenu.selectedText = text
                pageContextMenu.isEditable = editable
                pageContextMenu.popup(targetView, x, y)
            }
        }
    }
    MouseArea {
        // Reports a left press over the page to the current tab (for the external-link hint) and declines it,
        // so the press and its release still reach the web view underneath.
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: function(mouse) { if (currentView) currentView.noteLinkPress(); mouse.accepted = false }
    }
    Menu {
        id: pageContextMenu
        property string linkUrl: ""
        property string selectedText: ""
        property bool isEditable: false
        padding: 6
        background: Rectangle {
            implicitWidth: 230
            radius: 10
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#E00E2648" }
                GradientStop { position: 1.0; color: "#F0061224" }
            }
            border.color: CybouTheme.borderHighlight
            border.width: 1.5
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#90000000"
                shadowBlur: 1.0
                shadowVerticalOffset: 10
            }
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                height: 1; color: CybouTheme.glassSheen; radius: parent.radius
            }
        }
        GlassMenuItem {
            visible: pageContextMenu.linkUrl.length > 0 && /^https?:\/\//i.test(pageContextMenu.linkUrl)
            height: visible ? implicitHeight : 0
            text: qsTr("Ouvrir le lien dans un nouvel onglet")
            onTriggered: window.addTab(pageContextMenu.linkUrl)
        }
        GlassMenuItem {
            visible: pageContextMenu.linkUrl.length > 0
            height: visible ? implicitHeight : 0
            text: qsTr("Copier l’adresse du lien")
            onTriggered: if (currentView) currentView.triggerWebAction(WebEngineView.CopyLinkToClipboard)
        }
        GlassMenuSeparator {
            visible: pageContextMenu.linkUrl.length > 0
            height: visible ? implicitHeight : 0
        }
        GlassMenuItem {
            visible: pageContextMenu.selectedText.length > 0
            height: visible ? implicitHeight : 0
            text: qsTr("Copier")
            onTriggered: if (currentView) currentView.triggerWebAction(WebEngineView.Copy)
        }
        GlassMenuItem {
            visible: pageContextMenu.isEditable
            height: visible ? implicitHeight : 0
            text: qsTr("Coller")
            onTriggered: if (currentView) currentView.triggerWebAction(WebEngineView.Paste)
        }
        GlassMenuItem {
            text: qsTr("Retour")
            enabled: currentView ? currentView.canGoBack : false
            onTriggered: if (currentView) currentView.goBack()
        }
        GlassMenuItem {
            text: qsTr("Suivant")
            enabled: currentView ? currentView.canGoForward : false
            onTriggered: if (currentView) currentView.goForward()
        }
        GlassMenuItem {
            text: qsTr("Actualiser")
            onTriggered: if (currentView) currentView.reload()
        }
        GlassMenuItem {
            text: qsTr("Enregistrer en PDF…")
            enabled: currentView && !currentView.loading && currentView.url.toString() !== "about:blank" && !pdfBusy
            onTriggered: window.exportPdf()
        }
    }
    Connections { target: currentView; function onUrlChanged() { window.refreshAddress() } }
    NewTabPage {
        id: newTabPage
        anchors.fill: parent
        visible: currentView ? (currentView.url.toString() === "about:blank" || currentView.url.toString() === "") : true
        privateMode: window.privateMode
        onNavigateRequested: function(text) { window.navigate(text) }
        onLibraryRequested: function(section) { window.showLibrary(section) }
        onSettingsRequested: settingsPanel.open()
    }
    Dialog {
        id: behaviorDialog
        property string policyKey: ""
        property int policyMode: 0
        property string policyOrigin: ""
        title: qsTr("Modifier la politique de ce site ?")
        width: 430; anchors.centerIn: parent; modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: GlassDialogBackground {}
        header: Label { textFormat: Text.PlainText; text: behaviorDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { width: parent.width; wrapMode: Text.WordWrap; textFormat: Text.PlainText; color: CybouTheme.mainText; text: behaviorDialog.policyOrigin + "\n\n" + (behaviorDialog.policyKey === "popups" ? qsTr("Ce choix modifie l’ouverture de nouvelles fenêtres pour cette origine. Autoriser inclut les fenêtres sans geste utilisateur.") : qsTr("Le changement sera appliqué lors de la prochaine navigation ou après un rechargement explicite. Bloquer JavaScript ou localStorage peut perturber les connexions et paiements.")) }
        onAccepted: if (currentView && browserStore.origin(currentView.url.toString()) === policyOrigin) {
            if (!controlState.setBehavior(policyKey, policyMode)) window.showStatus(qsTr("La modification n’a pas pu être confirmée."))
        }
    }
    LibraryDrawer {
        id: library
        browserWindow: window
        settingsPanelRef: settingsPanel
        librarySection: window.librarySection
        downloadItems: window.downloadItems
        onLibrarySectionChanged: window.librarySection = librarySection
        onOpenUrlRequested: function(url) { addTab(url) }
        onReviewDownloadRequested: function(download) { reviewDownloadedFile(download) }
        onClearHistoryRequested: clearHistoryDialog.open()
    }
    Popup {
        id: suggestionsPopup
        parent: addressContainer
        x: 0; y: addressContainer.height + 6
        width: addressContainer.width
        padding: 6; focus: false; modal: false
        background: Rectangle {
            radius: CybouTheme.controlRadius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#F00E223D" }
                GradientStop { position: 1.0; color: "#F8061120" }
            }
            border.color: CybouTheme.borderHighlight; border.width: 1
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#80000000"
                shadowBlur: 1.0
                shadowVerticalOffset: 12
            }
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                height: 1; color: "#40FFFFFF"; radius: parent.radius
            }
        }
        onClosed: suggestionIndex = -1
        contentItem: Column {
            spacing: 2
            Repeater {
                model: suggestionRows
                delegate: Button {
                    id: suggestionButton
                    required property var modelData
                    required property int index
                    width: suggestionsPopup.availableWidth; height: 48
                    hoverEnabled: true; focusPolicy: Qt.NoFocus
                    Accessible.name: modelData.title + " " + modelData.url
                    background: Rectangle { radius: 7; color: suggestionIndex === suggestionButton.index || suggestionButton.hovered ? CybouTheme.glassHover : "transparent" }
                    contentItem: RowLayout {
                        spacing: 12
                        Label { textFormat: Text.PlainText; text: suggestionButton.modelData.kindId === "bookmark" || suggestionButton.modelData.kind === "Favori" ? "☆" : "↶"; color: CybouTheme.secondaryText; font.pixelSize: 20 }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 2
                            Label { Layout.fillWidth: true; text: suggestionButton.modelData.title; textFormat: Text.PlainText; elide: Text.ElideRight; color: CybouTheme.mainText; font.pixelSize: 12 }
                            Label { Layout.fillWidth: true; text: suggestionButton.modelData.url; textFormat: Text.PlainText; elide: Text.ElideRight; color: CybouTheme.secondaryText; font.pixelSize: 11 }
                        }
                        Label { textFormat: Text.PlainText; text: suggestionButton.modelData.kind; color: CybouTheme.mutedText; font.pixelSize: 10 }
                    }
                    onClicked: window.navigate(modelData.url)
                }
            }
        }
    }
    DownloadOpenDialog {
        id: downloadOpenDialog
        pendingOpenPath: window.pendingOpenPath
        pendingOpenOrigin: window.pendingOpenOrigin
        onStatusMessage: function(msg) { window.showStatus(msg) }
        onClosed: { window.pendingOpenPath = ""; window.pendingOpenOrigin = "" }
    }
    NativeDialogs.FileDialog {
        id: openFileDialog
        title: "Ouvrir un fichier"
        fileMode: NativeDialogs.FileDialog.OpenFile
        nameFilters: ["Documents web (*.html *.htm *.txt *.pdf)", "Images (*.png *.jpg *.jpeg *.gif *.webp *.svg)"]
        onAccepted: window.openLocalFile(selectedFile)
    }
    NativeDialogs.FileDialog {
        id: saveDownloadDialog
        title: "Enregistrer le téléchargement"
        fileMode: NativeDialogs.FileDialog.SaveFile
        onAccepted: window.finishDownloadChoice(selectedFile)
        onRejected: window.finishDownloadChoice(null)
    }
    NativeDialogs.FileDialog {
        id: pdfDialog
        title: "Enregistrer la page en PDF"
        fileMode: NativeDialogs.FileDialog.SaveFile
        nameFilters: ["Document PDF (*.pdf)"]
        defaultSuffix: "pdf"
        onAccepted: {
            const path = browserStore.localPath(selectedFile)
            if (pdfTarget && path.length) { pdfBusy = true; pdfTarget.printToPdf(path) }
            else { pdfTarget = null; window.showStatus("La page a été fermée.") }
        }
        onRejected: pdfTarget = null
    }
    Dialog {
        id: closeWorkDialog
        anchors.centerIn: parent; width: 420; modal: true
        title: "Fermer cette fenêtre ?"
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: GlassDialogBackground {}
        header: Label { textFormat: Text.PlainText; text: closeWorkDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { width: parent.width; wrapMode: Text.WordWrap; color: CybouTheme.mainText; text: "Des téléchargements ou un export PDF sont en cours. Fermer cette fenêtre les interrompra." }
        onAccepted: {
            for (const item of downloadItems) if (item && !item.isFinished) item.cancel()
            downloadQueue = []; pendingDownload = null
            saveDownloadDialog.close()
            closingApproved = true; window.close()
        }
    }
    Timer { id: statusTimer; interval: 6000; onTriggered: statusPopup.close() }
    Popup {
        id: statusPopup
        x: Math.max(12, (window.width - width) / 2)
        y: window.contentItem.height - height - 24
        width: Math.min(620, window.width - 32); padding: 14
        background: GlassDialogBackground { radius: 10 }
        contentItem: Label { id: statusLabel; textFormat: Text.PlainText; color: CybouTheme.mainText; wrapMode: Text.WordWrap; font.pixelSize: 12 }
    }
    SettingsPanel {
        id: settingsPanel
        workspaceTop: window.contentItem.y
        workspaceHeight: window.contentItem.height
        onAboutToShow: library.close()
        profile: window.currentProfile
        privateMode: window.privateMode
        onClearRequested: function(cookies, cache, history, permissions, session) { window.clearBrowsingData(cookies, cache, history, permissions, session) }
    }
    Dialog {
        id: clearHistoryDialog; anchors.centerIn: parent; title: "Effacer l’historique ?"; modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: GlassDialogBackground {}
        header: Label { textFormat: Text.PlainText; text: clearHistoryDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { text: "Toutes les visites enregistrées seront supprimées."; color: CybouTheme.mainText }
        onAccepted: browserStore.clearHistory()
    }
    Dialog {
        id: connectionInfo; anchors.centerIn: parent; width: 400; title: "Connexion"; modal: true; standardButtons: Dialog.Ok
        background: GlassDialogBackground {}
        header: Label { textFormat: Text.PlainText; text: connectionInfo.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { textFormat: Text.PlainText; width: parent.width; wrapMode: Text.WordWrap; color: CybouTheme.mainText; text: currentView && currentView.url.toString().startsWith("https://") ? "Cette page utilise HTTPS. Le chiffrement ne garantit pas la fiabilité du site." : "Cette page n’utilise pas HTTPS ou affiche la page d’accueil locale." }
    }
    Dialog {
        id: about; anchors.centerIn: parent; width: 440; title: "CYBOU 0.8.0"; modal: true; standardButtons: Dialog.Ok
        background: GlassDialogBackground {}
        header: Label { textFormat: Text.PlainText; text: about.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { width: parent.width; wrapMode: Text.WordWrap; color: CybouTheme.mainText; text: "Navigateur Qt Quick + Qt WebEngine.\n\nVersion de développement. Les sites demandent votre accord pour les permissions prises en charge. Le mode privé n’anonymise pas le trafic réseau. Les fichiers téléchargés restent sur le disque.\n\nCtrl+T : nouvel onglet · Ctrl+L : adresse\nCtrl+D : favori · Ctrl+F : recherche\nCtrl+H : historique · Ctrl+J : téléchargements\nCtrl+, : paramètres · Ctrl+Shift+N : fenêtre privée" }
    }
    Popup {
        id: pageLoadErrorDialog; objectName: "pageLoadErrorDialog"; anchors.centerIn: parent; width: Math.min(520, window.width - 40); padding: 24; modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: GlassDialogBackground { radius: 16 }
        contentItem: Label { id: errorText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: CybouTheme.mainText }
    }
    // System resizing preserves native platform behavior without a separate title bar.
    Repeater {
        model: [Qt.LeftEdge, Qt.RightEdge, Qt.TopEdge, Qt.BottomEdge, Qt.TopEdge | Qt.LeftEdge, Qt.TopEdge | Qt.RightEdge, Qt.BottomEdge | Qt.LeftEdge, Qt.BottomEdge | Qt.RightEdge]
        delegate: MouseArea {
            required property int modelData
            parent: window.contentItem
            z: 1000
            visible: window.visibility === Window.Windowed
            readonly property bool edgeleft: (modelData & Qt.LeftEdge) !== 0
            readonly property bool edgeright: (modelData & Qt.RightEdge) !== 0
            readonly property bool edgetop: (modelData & Qt.TopEdge) !== 0
            readonly property bool edgebottom: (modelData & Qt.BottomEdge) !== 0
            x: edgeleft ? 0 : edgeright ? window.width - width : 5
            y: edgetop ? -window.header.height : edgebottom ? window.contentItem.height - height : -window.header.height + 5
            width: edgeleft || edgeright ? 5 : window.width - 10
            height: edgetop || edgebottom ? 5 : window.height - 10
            cursorShape: (edgeleft && edgetop) || (edgeright && edgebottom) ? Qt.SizeFDiagCursor : (edgeright && edgetop) || (edgeleft && edgebottom) ? Qt.SizeBDiagCursor : edgeleft || edgeright ? Qt.SizeHorCursor : Qt.SizeVerCursor
            onPressed: window.startSystemResize(modelData)
        }
    }
}
