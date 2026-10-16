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

    component GlassMenuItem: MenuItem {
        id: gitem
        implicitHeight: 32
        scale: down ? 0.98 : 1.0
        contentItem: Text {
            leftPadding: 8; rightPadding: 8
            text: gitem.text
            color: gitem.enabled ? (gitem.highlighted ? "#FFFFFF" : CybouTheme.mainText) : CybouTheme.mutedText
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 6
            gradient: Gradient {
                GradientStop { position: 0.0; color: gitem.highlighted ? "#50285285" : "transparent" }
                GradientStop { position: 1.0; color: gitem.highlighted ? "#40183864" : "transparent" }
            }
            border.color: gitem.highlighted ? CybouTheme.borderHighlight : "transparent"
            border.width: 1
            Rectangle {
                visible: gitem.highlighted
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                height: 1; color: "#35FFFFFF"; radius: parent.radius
            }
        }
    }

    component GlassMenuSeparator: MenuSeparator {
        padding: 4
        contentItem: Rectangle {
            implicitHeight: 1
            color: CybouTheme.borderSubtle
        }
    }

    component GlassButton: Button {
        id: gbtn
        scale: down ? 0.95 : hovered ? 1.03 : 1.0
        Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100 } }
        contentItem: Text {
            text: gbtn.text; color: gbtn.highlighted ? "#FFFFFF" : gbtn.enabled ? CybouTheme.mainText : CybouTheme.mutedText
            font.pixelSize: 13; font.bold: gbtn.highlighted || gbtn.hovered
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 8
            gradient: Gradient {
                GradientStop { position: 0.0; color: gbtn.highlighted ? "#E0183A68" : gbtn.down ? "#DD0D2038" : gbtn.hovered ? "#DD224572" : "#80142F52" }
                GradientStop { position: 1.0; color: gbtn.highlighted ? "#E80E2444" : gbtn.down ? "#EE071222" : gbtn.hovered ? "#EE163052" : "#900B1D35" }
            }
            border.color: gbtn.highlighted ? CybouTheme.borderActive : gbtn.hovered ? "#7DA5CC" : CybouTheme.borderHighlight
            border.width: 1
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                height: 1; color: gbtn.highlighted ? "#50FFFFFF" : CybouTheme.glassSheenSubtle; radius: parent.radius
            }
        }
    }

    component GlassDialogBackground: Rectangle {
        radius: CybouTheme.dialogRadius
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#E80E2648" }
            GradientStop { position: 1.0; color: "#F4061224" }
        }
        border.color: CybouTheme.borderHighlight
        border.width: 1.5
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#A0000000"
            shadowBlur: 1.0
            shadowVerticalOffset: 12
        }
        Rectangle {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
            height: 1; color: CybouTheme.glassSheen; radius: parent.radius
        }
    }
    property int activeTab: 0
    readonly property int chromeRowHeight: CybouTheme.chromeRowHeight
    readonly property int chromeControlHeight: CybouTheme.chromeControlHeight
    property var currentView: views.count > activeTab ? views.itemAt(activeTab) : null
    property bool privateMode: false
    property string initialUrlArgument: ""
    property string initialLibrarySection: ""
    property bool initialSettingsOpen: false
    property bool initialMenuOpen: false
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
    property string librarySection: "Favoris"
    property bool findVisible: false
    property string findResult: ""
    property bool bookmarked: false
    property bool restoring: true
    property real testControlBudget: -1
    readonly property var activeProfile: privateMode ? privateProfile : persistentProfile
    property var pendingPermission: null
    property var pendingPermissionView: null
    property string pendingTopOrigin: ""
    property string pendingRequestOrigin: ""
    property int nextTabUid: 0
    function applyViewPolicy(view, target) {
        view.settings.javascriptEnabled = controlState.settingFor(target, "javascript")
        view.settings.localStorageEnabled = controlState.settingFor(target, "storage")
        view.settings.javascriptCanOpenWindows = controlState.popupMode(target) !== 2
        view.policyNeedsReload = false
    }
    ControlBarController {
        id: controlState
        store: browserStore
        profile: window.activeProfile
        url: window.currentView ? window.currentView.url : "about:blank"
        tabId: window.currentView ? window.currentView.tabUid : ""
        javascriptEnabled: window.currentView ? window.currentView.settings.javascriptEnabled : true
        storageEnabled: window.currentView ? window.currentView.settings.localStorageEnabled : true
        reloadRequired: window.currentView ? window.currentView.policyNeedsReload : false
        onPolicyChanged: function(origin, key) {
            for (let i = 0; i < views.count; ++i) {
                const view = views.itemAt(i)
                if (!view || browserStore.origin(view.url.toString()) !== origin) continue
                if (key === "javascript" || key === "storage") view.policyNeedsReload = true
                if (key === "popups") view.settings.javascriptCanOpenWindows = controlState.popupMode(view.url) !== 2
            }
        }
    }
    function cancelPermission() {
        if (pendingPermission && pendingPermission.isValid) {
            controlState.finishPermission(pendingPermission, pendingPermissionView ? pendingPermissionView.tabUid : "", false)
            pendingPermission.deny()
        }
        if (pendingPermissionView) controlState.abandonPending(pendingPermissionView.tabUid)
        pendingPermission = null
        pendingPermissionView = null
        permissionDialog.close()
    }
    function resolvePermission(allow) {
        if (pendingPermission && pendingPermission.isValid) {
            const valid = allow && pendingPermissionView === currentView && pendingPermissionView.profile === activeProfile
                && browserStore.origin(currentView.url.toString()) === pendingTopOrigin
                && browserStore.origin(pendingPermission.origin.toString()) === pendingRequestOrigin
            controlState.finishPermission(pendingPermission, pendingPermissionView ? pendingPermissionView.tabUid : "", valid)
            if (valid) pendingPermission.grant(); else pendingPermission.deny()
        }
        if (pendingPermissionView) controlState.abandonPending(pendingPermissionView.tabUid)
        pendingPermission = null; pendingPermissionView = null; permissionDialog.close()
    }
    function askPermission(permission, view) {
        if (!permission.isValid) return
        const types = WebEnginePermission.PermissionType
        const supported = [types.MediaAudioCapture, types.MediaVideoCapture, types.MediaAudioVideoCapture, types.MouseLock, types.Notifications, types.Geolocation, types.ClipboardReadWrite]
        const blocked = view !== currentView || pendingPermission || !supported.includes(permission.permissionType) || controlState.shouldDeny(permission)
        controlState.notePermission(permission, view.tabUid, view.url, !!blocked)
        if (blocked) {
            permission.deny(); return
        }
        pendingPermission = permission
        pendingPermissionView = view
        pendingTopOrigin = browserStore.origin(view.url.toString())
        pendingRequestOrigin = browserStore.origin(permission.origin.toString())
        permissionDialog.open()
    }
    function clearBrowsingData(cookies, cache, history, permissions, session) {
        cancelPermission()
        if (cookies || session) {
            tabs.clear(); closedTabs = []; addTab("")
        }
        if (history && !privateMode) browserStore.clearHistory()
        if (session && !privateMode) browserStore.saveSession([])
        if (permissions) {
            const decisions = activeProfile.listAllPermissions()
            for (const decision of decisions) decision.reset()
            controlState.clearPermissionPolicies()
        }
        if (cookies) browserStore.clearCookies(activeProfile)
        if (cache) browserStore.clearCache(activeProfile)
        settingsPanel.statusText = "Effacement demandé pour les catégories sélectionnées. Les cookies sont supprimés en arrière-plan."
        settingsPanel.refreshPermissions()
    }
    function runPrivateSmokeTest() {
        return privateMode && privateProfile && privateProfile.offTheRecord && persistentProfile === null && tabs.count === 1
    }
    function prepareDragSmokeTest() {
        controlBar.dismiss()
        library.close()
        tabs.clear()
        for (let i = 0; i < 3; ++i) addTab(smokeBaseUrl + "/drag" + i)
        tabStrip.forceLayout()
        const first = tabStrip.itemAtIndex(0)
        const last = tabStrip.itemAtIndex(2)
        const start = first.mapToItem(null, first.width / 2, first.height / 2)
        const end = last.mapToItem(null, last.width / 2 + 10, last.height / 2)
        return { x1: start.x, y1: start.y, x2: end.x, y2: end.y }
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
    function cleanFeaturePreview() {
        const url = smokeBaseUrl + "/preview"
        if (browserStore.isBookmarked(url)) browserStore.toggleBookmark(url, "")
        tabs.clear(); addTab(""); saveSession()
    }
    function testTabCount() { return tabs.count }
    function testAddressMatches() { return currentView && address.text === (currentView.url.toString() === "about:blank" ? "" : currentView.url.toString()) }
    function testOverlayVisibility() {
        settingsPanel.open(); const settingsHidden = !controlBar.interactionsEnabled && !controlBar.hasPopup; settingsPanel.close()
        library.open(); const libraryHidden = !controlBar.interactionsEnabled && !controlBar.hasPopup; library.close()
        window.showFullScreen(); const fullscreenHidden = !controlBar.interactionsEnabled && !controlBar.hasPopup; window.showNormal()
        return settingsHidden && libraryHidden && fullscreenHidden
    }
    function testOmnibox() {
        return { navWidth:navRow.width, fixedWidth:6 * 30 + 7 * navRow.spacing,
            addressWidth:addressContainer.width, editing:address.activeFocus,
            visibleKeys:controlBar.visibleKeys, overflowKeys:controlBar.hiddenKeys,
            overflowVisible:controlBar.overflowVisible, controlWidth:controlBar.effectiveWidth,
            downloadX:downloadsButton.x, menuX:menuButton.x, currentTabId:currentView ? currentView.tabUid : "",
            displayHost:address.displayHost, rawUrl:currentView ? currentView.url.toString() : "",
            input:address.text, selectedText:address.selectedText }
    }
    function testAddressPoint() { const p = address.mapToItem(null, address.width / 2, address.height / 2); return {kind:"click",x:p.x,y:p.y} }
    function testDraft(value) { address.text = value }
    function testLocalTextFile() { for (const item of downloadItems) if (item.downloadFileName.endsWith(".txt")) return browserStore.fileUrl(downloadPath(item)); return "" }
    function testBrowse() { address.focus = false; if (currentView) currentView.forceActiveFocus(); refreshAddress() }
    function beginControlBarTest() { controlHarness.begin() }
    function pollControlBarTest() { return controlHarness.step() }
    function takeControlBarScreenshot() { return controlHarness.takeScreenshot() }
    function takeControlBarInput() { return controlHarness.takeInput() }
    ControlBarHarness { id: controlHarness; browserWindow: window; controller: controlState; bar: controlBar }
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
            if (activeProfile.queryPermission(smokeBaseUrl, WebEnginePermission.PermissionType.Notifications).state !== WebEnginePermission.State.Granted) return -1
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
        currentView.url = target
        currentView.forceActiveFocus()
    }
    function refreshAddress() {
        if (!address.activeFocus) address.text = currentView && currentView.url.toString() !== "about:blank" ? currentView.url.toString() : ""
        bookmarked = currentView ? browserStore.isBookmarked(currentView.url.toString()) : false
    }
    function bookmark() {
        if (currentView) browserStore.toggleBookmark(currentView.url.toString(), currentView.title)
    }
    function showLibrary(section) { librarySection = section; library.open() }
    function maximizeToggle() { visibility === Window.Maximized ? showNormal() : showMaximized() }
    function handleDownload(download) {
        download.downloadDirectory = smokeMode ? smokeDownloadPath : browserStore.downloadsPath
        downloadItems = [download].concat(downloadItems)
        showLibrary("Téléchargements")
        if (browserStore.askDownloadLocation && !smokeMode) {
            downloadQueue = downloadQueue.concat([download]); presentNextDownload()
        } else download.accept()
    }
    Timer { id: saveTimer; interval: 700; onTriggered: saveSession() }
    onActiveTabChanged: { refreshAddress(); findVisible = false; cancelPermission(); if (controlBar) controlBar.dismiss() }
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
            persistentPermissionsPolicy: WebEngineProfile.StoreOnDisk
        }
    }
    Component { id: privatePrototype; WebEngineProfilePrototype {} }
    Connections { target: persistentProfile; function onDownloadRequested(download) { window.handleDownload(download) } }
    Connections { target: privateProfile; function onDownloadRequested(download) { window.handleDownload(download) } }
    ListModel { id: tabs }
    Component.onCompleted: {
        if (privateMode) privateProfile = privatePrototype.createObject(window).instance()
        else persistentProfile = normalPrototype.createObject(window).instance()
        browserStore.configureProfile(activeProfile)
        const saved = !privateMode && !smokeMode && browserStore.restoreSession ? browserStore.session() : []
        for (let i = 0; i < saved.length; ++i) restoreTab(saved[i])
        if (initialUrlArgument.length > 0) addTab(initialUrlArgument)
        else if (!tabs.count) addTab("")
        restoring = false
        if (initialLibrarySection.length > 0) showLibrary(initialLibrarySection)
        if (initialSettingsOpen) settingsPanel.open()
        if (initialMenuOpen) menu.popup(menuButton, 0, menuButton.height)
    }
    Shortcut { sequence: "Ctrl+L"; onActivated: { address.forceActiveFocus(); address.selectAll() } }
    Shortcut { sequence: "Ctrl+T"; onActivated: addTab("") }
    Shortcut { sequence: "Ctrl+W"; onActivated: closeTab(activeTab) }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: reopenTab() }
    Shortcut { sequence: "Ctrl+Tab"; onActivated: activeTab = (activeTab + 1) % tabs.count }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: activeTab = (activeTab + tabs.count - 1) % tabs.count }
    Shortcut { sequence: "Ctrl+R"; onActivated: if (currentView) currentView.reload() }
    Shortcut { sequence: "Ctrl+D"; onActivated: bookmark() }
    Shortcut { sequence: "Ctrl+H"; onActivated: showLibrary("Historique") }
    Shortcut { sequence: "Ctrl+J"; onActivated: showLibrary("Téléchargements") }
    Shortcut { sequence: "Ctrl+F"; onActivated: { findVisible = true; findInput.forceActiveFocus(); findInput.selectAll() } }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: browserStore.newPrivateWindow() }
    Shortcut { sequence: "Ctrl+,"; onActivated: settingsPanel.open() }
    Shortcut { sequence: "Ctrl+O"; onActivated: openFileDialog.open() }
    Shortcut { sequence: "Ctrl+P"; onActivated: exportPdf() }
    Shortcut { sequence: "Ctrl+M"; onActivated: toggleMute(activeTab) }
    Shortcut { sequence: "Alt+Left"; onActivated: if (currentView) currentView.goBack() }
    Shortcut { sequence: "Alt+Right"; onActivated: if (currentView) currentView.goForward() }
    Shortcut { sequence: "F11"; onActivated: visibility === Window.FullScreen ? showNormal() : showFullScreen() }
    Shortcut { sequence: "Escape"; enabled: !address.activeFocus && !controlBar.hasPopup; onActivated: { findVisible = false; if (currentView) { currentView.findText(""); currentView.stop() } } }

    header: ColumnLayout {
        spacing: 0
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: chromeRowHeight; Layout.minimumHeight: chromeRowHeight; Layout.maximumHeight: chromeRowHeight
            color: privateMode ? "#060F1E" : "#081325"
            MouseArea { anchors.fill: parent; onPressed: window.startSystemMove(); onDoubleClicked: window.maximizeToggle() }
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
                    boundsBehavior: Flickable.StopAtBounds
                    model: tabs
                    delegate: Rectangle {
                        id: tabItem
                        required property int index
                        required property string label
                        required property string favicon
                        required property bool isPinned
                        required property bool isMuted
                        required property bool audible
                        width: isPinned ? (isMuted || audible ? 66 : 42) : Math.max(112, Math.min(200, tabStrip.width / Math.min(tabs.count, 5)))
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
                            Text { visible: !tabItem.isPinned; text: tabItem.label; elide: Text.ElideRight; Layout.fillWidth: true; color: activeTab === tabItem.index ? "#FFFFFF" : "#8FA3BF"; font.bold: activeTab === tabItem.index; font.pixelSize: 12 }
                            NavButton {
                                visible: tabItem.isMuted || tabItem.audible; implicitWidth: 22; implicitHeight: 24
                                hint: tabItem.isMuted ? "Réactiver le son" : "Couper le son"
                                contentItem: Image { source: tabItem.isMuted ? "qrc:/assets/speaker-muted.svg" : "qrc:/assets/speaker.svg"; sourceSize.width: 16; sourceSize.height: 16; fillMode: Image.PreserveAspectFit }
                                onClicked: toggleMute(tabItem.index)
                            }
                            NavButton { visible: !tabItem.isPinned; text: "×"; implicitWidth: 22; implicitHeight: 24; hint: "Fermer · Ctrl+W"; onClicked: closeTab(tabItem.index) }
                        }
                        ToolTip.visible: tabMouse.containsMouse && !tabMouse.pressed
                        ToolTip.delay: 700
                        ToolTip.text: tabItem.label
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
                            GlassMenuItem { text: tabItem.isPinned ? "Désépingler l’onglet" : "Épingler l’onglet"; onTriggered: togglePin(tabItem.index) }
                            GlassMenuItem { text: tabItem.isMuted ? "Réactiver le son" : "Couper le son"; onTriggered: toggleMute(tabItem.index) }
                            GlassMenuItem { text: "Déplacer à gauche"; enabled: tabItem.index > 0; onTriggered: moveTab(tabItem.index, tabItem.index - 1) }
                            GlassMenuItem { text: "Déplacer à droite"; enabled: tabItem.index < tabs.count - 1; onTriggered: moveTab(tabItem.index, tabItem.index + 1) }
                            GlassMenuItem { text: "Dupliquer"; onTriggered: addTab(views.itemAt(tabItem.index).url.toString()) }
                            GlassMenuItem { text: "Fermer"; onTriggered: closeTab(tabItem.index) }
                            GlassMenuItem { text: "Rouvrir l’onglet fermé"; enabled: closedTabs.length > 0; onTriggered: reopenTab() }
                        }
                    }
                }
                NavButton { text: "+"; hint: "Nouvel onglet · Ctrl+T"; onClicked: addTab("") }
                Label { visible: privateMode; text: "PRIVÉ"; color: "#8FA3BF"; font.pixelSize: 10; padding: 8 }
                Item { Layout.preferredWidth: 18 }
                NavButton { text: "−"; hint: "Réduire"; implicitWidth: 38; implicitHeight: chromeRowHeight; onClicked: window.showMinimized() }
                NavButton { text: window.visibility === Window.Maximized ? "❐" : "□"; hint: "Agrandir / restaurer"; implicitWidth: 38; implicitHeight: chromeRowHeight; onClicked: window.maximizeToggle() }
                Button {
                    id: closeWindow
                    text: "×"; implicitWidth: 38; implicitHeight: chromeRowHeight; padding: 0; hoverEnabled: true
                    Accessible.name: "Fermer CYBOU"
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
                NavButton { text: "←"; hint: "Retour"; enabled: currentView ? currentView.canGoBack : false; onClicked: currentView.goBack() }
                NavButton { text: "→"; hint: "Suivant"; enabled: currentView ? currentView.canGoForward : false; onClicked: currentView.goForward() }
                NavButton { text: currentView && currentView.loading ? "×" : "↻"; hint: "Actualiser / arrêter"; onClicked: currentView.loading ? currentView.stop() : currentView.reload() }
                NavButton { text: "⌂"; hint: "Accueil"; onClicked: currentView.url = "about:blank" }
                Rectangle {
                    id: addressContainer
                    Layout.fillWidth: true; Layout.minimumWidth: 160; implicitHeight: chromeControlHeight
                    radius: 8
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: address.activeFocus ? "#E80B1E38" : "#CC0F2748" }
                        GradientStop { position: 1.0; color: address.activeFocus ? "#F4061324" : "#DD091B32" }
                    }
                    border.width: address.activeFocus ? 1.5 : 1
                    border.color: address.activeFocus ? "#0088FF" : "#357DA5CC"
                    Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150 } }
                    Rectangle {
                        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                        height: 1; color: address.activeFocus ? "#4038BDF8" : "#25FFFFFF"; radius: parent.radius
                    }
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 5; anchors.rightMargin: 4; spacing: 3
                        NavButton {
                            text: currentView && currentView.url.toString().startsWith("https:") ? "🔒" : currentView && currentView.url.toString().startsWith("http:") ? "!" : "ⓘ"
                            implicitWidth: 24
                            hint: currentView && currentView.url.toString().startsWith("https:") ? qsTr("Connexion chiffrée (HTTPS), sans garantie sur le site") : currentView && currentView.url.toString().startsWith("http:") ? qsTr("Connexion non chiffrée (HTTP)") : qsTr("Page locale ou nouvel onglet")
                            onClicked: connectionInfo.open()
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
                ControlBarInline {
                    id: controlBar
                    controller: controlState; popupHost: Overlay.overlay
                    editing: address.activeFocus
                    widthBudget: smokeMode && window.testControlBudget >= 0 ? window.testControlBudget : Math.max(0, navRow.width - 6 * 30 - 7 * navRow.spacing - 160)
                    Layout.preferredWidth: effectiveWidth; Layout.minimumWidth: effectiveWidth; Layout.maximumWidth: effectiveWidth
                    visible: !editing
                    interactionsEnabled: window.visibility !== Window.FullScreen && !library.visible && !settingsPanel.visible && !behaviorDialog.visible && !errorNotice.visible && !closeWorkDialog.visible && !downloadOpenDialog.visible && !permissionDialog.visible
                    onPolicyRequested: function(key, mode) {
                        if (key === "error") { window.showStatus(qsTr("La modification n’a pas pu être confirmée.")); return }
                        behaviorDialog.policyKey = key; behaviorDialog.policyMode = mode
                        behaviorDialog.policyOrigin = browserStore.origin(currentView.url.toString())
                        behaviorDialog.open()
                    }
                    onDecisionRequested: function(allow) { window.resolvePermission(allow) }
                    onReloadRequested: if (currentView) { window.applyViewPolicy(currentView, currentView.url); currentView.reload(); controlBar.dismiss() }
                }
                NavButton { id: downloadsButton; text: "↓"; hint: "Téléchargements · Ctrl+J"; onClicked: showLibrary("Téléchargements") }
                NavButton { id: menuButton; text: "☰"; hint: "Menu"; onClicked: menu.popup(menuButton, 0, menuButton.height) }
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
                    GlassMenuItem { text: "Nouvel onglet                         Ctrl+T"; onTriggered: addTab("") }
                    GlassMenuItem { text: "Rouvrir l’onglet fermé          Ctrl+Shift+T"; enabled: closedTabs.length > 0; onTriggered: reopenTab() }
                    GlassMenuItem { text: "Nouvelle fenêtre privée     Ctrl+Shift+N"; onTriggered: browserStore.newPrivateWindow() }
                    GlassMenuSeparator {}
                    GlassMenuItem { text: "Favoris"; onTriggered: showLibrary("Favoris") }
                    GlassMenuItem { text: "Historique                                Ctrl+H"; onTriggered: showLibrary("Historique") }
                    GlassMenuItem { text: "Téléchargements                      Ctrl+J"; onTriggered: showLibrary("Téléchargements") }
                    GlassMenuItem { text: "Rechercher dans la page           Ctrl+F"; onTriggered: { findVisible = true; findInput.forceActiveFocus() } }
                    GlassMenuItem { text: "Ouvrir un fichier…                        Ctrl+O"; onTriggered: openFileDialog.open() }
                    GlassMenuItem { text: "Enregistrer en PDF…                   Ctrl+P"; enabled: currentView && !currentView.loading && currentView.url.toString() !== "about:blank" && !pdfBusy; onTriggered: exportPdf() }
                    GlassMenuSeparator {}
                    GlassMenuItem { text: "Zoom +"; onTriggered: currentView.zoomFactor = Math.min(3, currentView.zoomFactor + 0.1) }
                    GlassMenuItem { text: "Zoom −"; onTriggered: currentView.zoomFactor = Math.max(0.3, currentView.zoomFactor - 0.1) }
                    GlassMenuItem { text: "Zoom 100 %"; onTriggered: currentView.zoomFactor = 1 }
                    GlassMenuItem { text: "Plein écran                                  F11"; onTriggered: window.visibility === Window.FullScreen ? window.showNormal() : window.showFullScreen() }
                    GlassMenuSeparator {}
                    GlassMenuItem { text: "Paramètres                                     Ctrl+,"; onTriggered: settingsPanel.open() }
                    GlassMenuItem { text: "À propos de CYBOU"; onTriggered: about.open() }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 2; color: "#11243E"
            Rectangle { height: 2; width: parent.width * (currentView ? currentView.loadProgress / 100 : 0); color: "#0088FF"; visible: currentView ? currentView.loading : false }
        }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: findVisible ? 48 : 0; visible: findVisible; color: "#0A182F"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#11243E" }
            RowLayout {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 12; spacing: 8
                TextField {
                    id: findInput; placeholderText: "Rechercher dans la page"; color: "white"
                    placeholderTextColor: "#64748B"
                    background: Rectangle { radius: 6; color: "#0B1E38"; border.color: "#1B3B66" }
                    onTextChanged: if (currentView) currentView.findText(text); onAccepted: if (currentView) currentView.findText(text)
                }
                Label { text: findResult; color: "#8FA3BF" }
                NavButton { text: "↑"; hint: "Précédent"; onClicked: currentView.findText(findInput.text, WebEngineView.FindBackward) }
                NavButton { text: "↓"; hint: "Suivant"; onClicked: currentView.findText(findInput.text) }
                NavButton { text: "×"; hint: "Fermer la recherche"; onClicked: { findVisible = false; currentView.findText("") } }
            }
        }
    }
    Repeater {
        id: views; model: tabs
        delegate: WebEngineView {
            id: web
            required property int index
            required property string initialUrl
            required property bool isMuted
            required property string tabUid
            property bool policyNeedsReload: false
            anchors.fill: parent
            visible: activeTab === index && url.toString() !== "about:blank"
            url: initialUrl
            profile: privateMode ? privateProfile : persistentProfile
            audioMuted: isMuted
            onRecentlyAudibleChanged: function(value) { if (index < tabs.count) tabs.setProperty(index, "audible", value) }
            settings.javascriptEnabled: controlState.settingFor(initialUrl, "javascript")
            settings.localStorageEnabled: controlState.settingFor(initialUrl, "storage")
            settings.javascriptCanOpenWindows: controlState.popupMode(initialUrl) !== 2
            settings.localContentCanAccessRemoteUrls: false
            settings.localContentCanAccessFileUrls: false
            settings.allowRunningInsecureContent: false
            settings.unknownUrlSchemePolicy: WebEngineSettings.DisallowUnknownUrlSchemes
            settings.webRTCPublicInterfacesOnly: true
            settings.fullScreenSupportEnabled: true
            onNavigationRequested: function(request) {
                if (pendingPermissionView === web) window.cancelPermission()
                const reason = browserStore.navigationBlockReason(request.url)
                if (reason.length > 0) {
                    request.reject()
                    if (request.isMainFrame && web === currentView) { errorText.text = reason; errorNotice.open() }
                    return
                }
                window.applyViewPolicy(web, request.url)
            }
            onFullScreenRequested: function(request) { request.accept(); request.toggleOn ? window.showFullScreen() : window.showNormal() }
            onTitleChanged: if (index < tabs.count) tabs.setProperty(index, "label", url.toString() === "about:blank" ? "Nouvel onglet" : title || "Nouvel onglet")
            onIconChanged: if (index < tabs.count) tabs.setProperty(index, "favicon", icon.toString())
            onUrlChanged: { if (pendingPermissionView === web) cancelPermission(); if (index === activeTab) refreshAddress(); saveTimer.restart() }
            onCertificateError: function(error) { error.rejectCertificate() }
            onPdfPrintingFinished: function(path, success) {
                pdfBusy = false; pdfTarget = null
                showStatus(success ? "PDF enregistré : " + path : "Échec de l’enregistrement PDF.")
                if (smokeMode) smokePdfResult = success ? 1 : -1
            }
            onPermissionRequested: function(permission) { window.askPermission(permission, web) }
            onFindTextFinished: function(result) { if (index === activeTab) findResult = result.activeMatch + " / " + result.numberOfMatches }
            onNewWindowRequested: function(request) {
                const mode = controlState.popupMode(url)
                const permitted = mode !== 2 && (request.userInitiated || mode === 1) && /^https?:\/\//i.test(request.requestedUrl.toString())
                controlState.notePopup(tabUid, url, !permitted)
                if (permitted) addTab(request.requestedUrl.toString())
            }
            onLoadingChanged: function(info) {
                if (smokeMode && url.toString().startsWith("file:") && info.status === WebEngineView.LoadSucceededStatus) smokeLocalLoaded = true
                if (info.status === WebEngineView.LoadSucceededStatus && !privateMode) browserStore.recordVisit(url.toString(), title)
                if (info.status === WebEngineView.LoadFailedStatus && index === activeTab) {
                    errorText.text = "Impossible de charger cette page. Vérifiez l’adresse et votre connexion.\n\n" + info.errorString
                    errorNotice.open()
                }
            }
        }
    }
    Connections { target: currentView; function onUrlChanged() { window.refreshAddress() } }
    NewTabPage {
        anchors.fill: parent
        visible: currentView ? (currentView.url.toString() === "about:blank" || currentView.url.toString() === "") : true
        privateMode: window.privateMode
        onNavigateRequested: function(text) { window.navigate(text) }
        onLibraryRequested: function(section) { window.showLibrary(section) }
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
        header: Label { text: behaviorDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { width: parent.width; wrapMode: Text.WordWrap; textFormat: Text.PlainText; color: CybouTheme.mainText; text: behaviorDialog.policyOrigin + "\n\n" + (behaviorDialog.policyKey === "popups" ? qsTr("Ce choix modifie l’ouverture de nouvelles fenêtres pour cette origine. Autoriser inclut les fenêtres sans geste utilisateur.") : qsTr("Le changement sera appliqué lors de la prochaine navigation ou après un rechargement explicite. Bloquer JavaScript ou localStorage peut perturber les connexions et paiements.")) }
        onAccepted: if (currentView && browserStore.origin(currentView.url.toString()) === policyOrigin) {
            if (!controlState.setBehavior(policyKey, policyMode)) window.showStatus(qsTr("La modification n’a pas pu être confirmée."))
        }
    }
    Drawer {
        id: library
        edge: Qt.RightEdge; width: Math.min(460, window.width - 60); height: window.height
        background: Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#D80A1F38" }
                GradientStop { position: 1.0; color: "#E8061224" }
            }
            border.color: CybouTheme.borderHighlight
            border.width: 1
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#90000000"
                shadowBlur: 1.0
                shadowHorizontalOffset: -12
            }
            Rectangle {
                anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                width: 1; color: CybouTheme.glassSheen
            }
        }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 14
            RowLayout {
                Label { text: librarySection; font.pixelSize: 23; font.bold: true; color: CybouTheme.mainText; Layout.fillWidth: true }
                NavButton { text: "×"; hint: "Fermer"; onClicked: library.close() }
            }
            RowLayout {
                spacing: 8
                GlassButton { text: "Favoris"; highlighted: librarySection === "Favoris"; onClicked: librarySection = "Favoris" }
                GlassButton { text: "Historique"; highlighted: librarySection === "Historique"; onClicked: librarySection = "Historique" }
                GlassButton { text: "Fichiers"; highlighted: librarySection === "Téléchargements"; onClicked: librarySection = "Téléchargements" }
            }
            TextField {
                id: librarySearch; Layout.fillWidth: true; placeholderText: "Filtrer…"
                visible: librarySection !== "Téléchargements"
                color: CybouTheme.mainText; placeholderTextColor: CybouTheme.mutedText
                background: Rectangle {
                    radius: 8
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: librarySearch.activeFocus ? "#E0143258" : "#C0102644" }
                        GradientStop { position: 1.0; color: librarySearch.activeFocus ? "#F00A1A30" : "#D008172A" }
                    }
                    border.color: librarySearch.activeFocus ? CybouTheme.borderActive : CybouTheme.borderHighlight
                    border.width: 1
                }
            }
            GlassButton { text: "Effacer l’historique"; visible: librarySection === "Historique"; onClicked: clearHistoryDialog.open() }
            GlassButton { text: "Ouvrir le dossier Téléchargements"; visible: librarySection === "Téléchargements"; onClicked: browserStore.openDownloads() }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 8
                model: librarySection === "Favoris" ? browserStore.bookmarks : librarySection === "Historique" ? browserStore.history : downloadItems
                delegate: Rectangle {
                    id: entry
                    required property var modelData
                    property bool download: librarySection === "Téléchargements"
                    property bool matches: download || (modelData.title + " " + modelData.url).toLowerCase().includes(librarySearch.text.toLowerCase())
                    width: ListView.view.width; height: matches ? (download ? 152 : 82) : 0; visible: matches
                    radius: 10
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#60142F52" }
                        GradientStop { position: 1.0; color: "#700A182E" }
                    }
                    border.color: CybouTheme.borderSubtle
                    border.width: 1
                    Rectangle {
                        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                        height: 1; color: "#25FFFFFF"; radius: parent.radius
                    }
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 12; spacing: 4
                        Label { Layout.fillWidth: true; text: entry.download ? entry.modelData.downloadFileName : entry.modelData.title; color: CybouTheme.mainText; elide: Text.ElideRight; font.bold: true }
                        Label { Layout.fillWidth: true; text: entry.download ? window.downloadStatus(entry.modelData) : entry.modelData.url; elide: Text.ElideRight; color: CybouTheme.secondaryText; font.pixelSize: 11 }
                        ProgressBar { visible: entry.download; Layout.fillWidth: true; from: 0; to: entry.download ? Math.max(1, entry.modelData.totalBytes) : 1; value: entry.download ? entry.modelData.receivedBytes : 0; indeterminate: entry.download && entry.modelData.totalBytes <= 0 && !entry.modelData.isFinished }
                        RowLayout {
                            spacing: 8
                            GlassButton { text: "Ouvrir"; visible: !entry.download || entry.modelData.state === WebEngineDownloadRequest.DownloadCompleted; onClicked: { if (entry.download) window.reviewDownloadedFile(entry.modelData); else { addTab(entry.modelData.url); library.close() } } }
                            GlassButton { text: "Dossier"; visible: entry.download; onClicked: browserStore.showFileFolder(window.downloadPath(entry.modelData)) }
                            GlassButton { text: "Retirer"; visible: librarySection === "Favoris"; onClicked: browserStore.toggleBookmark(entry.modelData.url, entry.modelData.title) }
                        }
                        RowLayout {
                            spacing: 8
                            visible: entry.download && !entry.modelData.isFinished
                            GlassButton { text: entry.download && entry.modelData.isPaused ? "Reprendre" : "Pause"; enabled: entry.download && entry.modelData.state === WebEngineDownloadRequest.DownloadInProgress; onClicked: entry.modelData.isPaused ? entry.modelData.resume() : entry.modelData.pause() }
                            GlassButton { text: "Annuler"; onClicked: entry.modelData.cancel() }
                        }
                    }
                }
                Label { anchors.centerIn: parent; visible: parent.count === 0; text: "Rien ici pour le moment"; color: "#8491A5" }
            }
        }
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
                        Label { text: suggestionButton.modelData.kind === "Favori" ? "☆" : "↶"; color: CybouTheme.secondaryText; font.pixelSize: 20 }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 2
                            Label { Layout.fillWidth: true; text: suggestionButton.modelData.title; textFormat: Text.PlainText; elide: Text.ElideRight; color: CybouTheme.mainText; font.pixelSize: 12 }
                            Label { Layout.fillWidth: true; text: suggestionButton.modelData.url; textFormat: Text.PlainText; elide: Text.ElideRight; color: CybouTheme.secondaryText; font.pixelSize: 11 }
                        }
                        Label { text: suggestionButton.modelData.kind; color: CybouTheme.mutedText; font.pixelSize: 10 }
                    }
                    onClicked: window.navigate(modelData.url)
                }
            }
        }
    }
    Dialog {
        id: downloadOpenDialog
        anchors.centerIn: parent; width: Math.min(500, window.width - 40); modal: true
        title: qsTr("Ouvrir un fichier téléchargé")
        padding: 20
        background: GlassDialogBackground {}
        header: Label { text: downloadOpenDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        property bool allowed: visible && browserStore.canOpenDownloadedFile(window.pendingOpenPath)
        contentItem: ColumnLayout {
            spacing: 14
            Label { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText; font.bold: true; color: CybouTheme.mainText; text: browserStore.fileName(browserStore.fileUrl(window.pendingOpenPath)) }
            Label { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText; color: CybouTheme.secondaryText; text: qsTr("Source : %1").arg(window.pendingOpenOrigin || qsTr("Source inconnue")) }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: CybouTheme.mainText; text: downloadOpenDialog.allowed ? qsTr("Ce fichier sera ouvert par une application externe. Continuez uniquement si vous faites confiance à sa source. CYBOU ne réalise pas d’analyse antivirus.") : qsTr("L’ouverture de ce type de fichier est bloquée dans CYBOU. Les programmes, scripts, raccourcis et types inconnus doivent être examinés depuis leur dossier.") }
            Button { text: qsTr("Afficher le dossier"); onClicked: { browserStore.showFileFolder(window.pendingOpenPath); downloadOpenDialog.reject() } }
        }
        footer: DialogButtonBox {
            background: Rectangle { color: "transparent"; radius: CybouTheme.dialogRadius }
            Button { text: qsTr("Annuler"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole; focus: true; onClicked: downloadOpenDialog.reject() }
            Button { text: qsTr("Ouvrir dans une application"); enabled: downloadOpenDialog.allowed; onClicked: { const path = window.pendingOpenPath; downloadOpenDialog.accept(); if (!browserStore.openDownloadedFile(path)) window.showStatus(qsTr("Impossible d’ouvrir ce fichier en toute sécurité.")) } }
        }
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
        header: Label { text: closeWorkDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
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
        contentItem: Label { id: statusLabel; color: CybouTheme.mainText; wrapMode: Text.WordWrap; font.pixelSize: 12 }
    }
    Dialog {
        id: permissionDialog
        anchors.centerIn: parent; width: Math.min(460, window.width - 40)
        title: "Autorisation du site"; modal: true
        closePolicy: Popup.CloseOnEscape
        background: GlassDialogBackground {}
        header: Label { text: permissionDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        contentItem: ColumnLayout {
            spacing: 14
            Label { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; font.bold: true; color: CybouTheme.mainText; text: pendingPermission ? pendingPermission.origin.toString() : "" }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: CybouTheme.mainText; text: pendingPermission ? "Ce site demande : " + browserStore.permissionName(pendingPermission.permissionType) : "" }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: CybouTheme.secondaryText; font.pixelSize: 12; text: privateMode ? "Décision limitée à cette fenêtre privée." : "Les décisions persistantes sont révocables dans Paramètres → Autorisations des sites." }
        }
        footer: DialogButtonBox {
            background: Rectangle { color: "transparent" }
            Button {
                text: "Refuser"; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: window.cancelPermission()
            }
            Button {
                text: "Autoriser"; DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: window.resolvePermission(true)
            }
        }
        onRejected: window.cancelPermission()
        onClosed: if (pendingPermission) window.cancelPermission()
    }
    SettingsPanel {
        id: settingsPanel
        profile: window.activeProfile
        privateMode: window.privateMode
        onClearRequested: function(cookies, cache, history, permissions, session) { window.clearBrowsingData(cookies, cache, history, permissions, session) }
    }
    Dialog {
        id: clearHistoryDialog; anchors.centerIn: parent; title: "Effacer l’historique ?"; modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: GlassDialogBackground {}
        header: Label { text: clearHistoryDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { text: "Toutes les visites enregistrées seront supprimées."; color: CybouTheme.mainText }
        onAccepted: browserStore.clearHistory()
    }
    Dialog {
        id: connectionInfo; anchors.centerIn: parent; width: 400; title: "Connexion"; modal: true; standardButtons: Dialog.Ok
        background: GlassDialogBackground {}
        header: Label { text: connectionInfo.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { width: parent.width; wrapMode: Text.WordWrap; color: CybouTheme.mainText; text: currentView && currentView.url.toString().startsWith("https://") ? "Cette page utilise HTTPS. Le chiffrement ne garantit pas la fiabilité du site." : "Cette page n’utilise pas HTTPS ou affiche la page d’accueil locale." }
    }
    Dialog {
        id: about; anchors.centerIn: parent; width: 440; title: "CYBOU 0.8.0"; modal: true; standardButtons: Dialog.Ok
        background: GlassDialogBackground {}
        header: Label { text: about.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        Label { width: parent.width; wrapMode: Text.WordWrap; color: CybouTheme.mainText; text: "Navigateur Qt Quick + Qt WebEngine.\n\nVersion de développement. Les sites demandent votre accord pour les permissions prises en charge. Le mode privé n’anonymise pas le trafic réseau. Les fichiers téléchargés restent sur le disque.\n\nCtrl+T : nouvel onglet · Ctrl+L : adresse\nCtrl+D : favori · Ctrl+F : recherche\nCtrl+H : historique · Ctrl+J : téléchargements\nCtrl+, : paramètres · Ctrl+Shift+N : fenêtre privée" }
    }
    Popup {
        id: errorNotice; anchors.centerIn: parent; width: Math.min(520, window.width - 40); padding: 24; modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: GlassDialogBackground { radius: 16 }
        contentItem: Label { id: errorText; wrapMode: Text.WordWrap; color: CybouTheme.mainText }
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
