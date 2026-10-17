import QtQuick
import QtWebEngine
import Cybou

WebEngineView {
    id: web
    required property int index
    required property string initialUrl
    required property bool isMuted
    required property string tabUid

    property string blockedHttpUrl: ""
    property int blockedHttpEpoch: -1
    readonly property bool temporaryContext: !!window.temporaryProfiles[tabUid]
    readonly property bool temporaryTransport: browserStore.isTemporaryHttpProfile(window.temporaryProfiles[tabUid] || null)
    property string policyOrigin: initialUrl !== "about:blank" ? browserStore.origin(initialUrl) : ""
    property int navigationEpoch: 0
    readonly property var grantedKeys: {
        permissionCoordinator.revision
        return permissionCoordinator.grantedKeys(web, navigationEpoch)
    }
    readonly property bool hasPermissionLease: grantedKeys.length > 0
    Component.onDestruction: {
        if (permissionCoordinator) permissionCoordinator.forgetView(web)
        if (controlState) controlState.expireContext(tabUid)
        browserStore.releaseTrackerOwner(web.profile, web)
        window.releaseTemporaryView(tabUid)
    }
    property bool policyNeedsReload: false
    property string upgradedFromHttpUrl: ""
    property string upgradeExpectedUrl: ""
    property int upgradeEpoch: -1
    anchors.fill: parent
    visible: activeTab === index && url.toString() !== "about:blank"
    url: initialUrl
    profile: window.temporaryProfiles[tabUid] || window.activeProfile
    Connections {
        target: web.temporaryContext ? web.profile : null
        function onDownloadRequested(download) { window.handleDownload(download) }
    }
    audioMuted: isMuted
    onRecentlyAudibleChanged: function(value) { if (index < tabs.count) tabs.setProperty(index, "audible", value) }
    settings.javascriptEnabled: controlState.settingFor(initialUrl, "javascript", tabUid)
    settings.localStorageEnabled: controlState.settingFor(initialUrl, "storage", tabUid)
    settings.javascriptCanOpenWindows: controlState.popupMode(initialUrl, tabUid) !== 2
    settings.localContentCanAccessRemoteUrls: false
    settings.localContentCanAccessFileUrls: false
    settings.allowRunningInsecureContent: false
    settings.unknownUrlSchemePolicy: WebEngineSettings.DisallowUnknownUrlSchemes
    settings.webRTCPublicInterfacesOnly: true
    settings.fullScreenSupportEnabled: true
    onNavigationRequested: function(request) {
        if (request.isMainFrame) {
            web.navigationEpoch++
            if (request.url.toString() !== web.upgradeExpectedUrl) {
                web.upgradedFromHttpUrl = ""
                web.upgradeExpectedUrl = ""
                web.upgradeEpoch = -1
            } else web.upgradeEpoch = web.navigationEpoch
            web.blockedHttpUrl = ""
            web.blockedHttpEpoch = -1
            if (web === currentView) { policyBanner.close(); controlBar.dismiss() }
            if (pendingPermissionView === web) window.cancelPermission()
            if (web.temporaryTransport && !browserStore.canStartTemporaryHttp(web.profile, request.url)) {
                request.reject()
                window.endTemporaryNavigation(web, request.url.toString())
                return
            }
            if (!web.temporaryTransport && browserStore.shouldUpgradeToHttps(request.url)) {
                request.reject()
                web.upgradedFromHttpUrl = request.url.toString()
                web.upgradeExpectedUrl = browserStore.upgradeToHttps(request.url).toString()
                web.url = web.upgradeExpectedUrl
                return
            }
        }
        const reason = browserStore.navigationBlockReason(request.url, web.profile, request.isMainFrame)
        if (reason.length > 0) {
            request.reject()
            if (request.isMainFrame && web === currentView) {
                if (browserStore.canRequestTemporaryHttp(request.url)) window.offerHttp(web, request.url)
                else { errorText.text = reason; errorNotice.open() }
            }
            return
        }
        if (request.isMainFrame) {
            const nextOrigin = browserStore.origin(request.url.toString())
            if (web.policyOrigin !== nextOrigin) controlState.expireContext(web.tabUid)
            web.policyOrigin = nextOrigin
            window.applyViewPolicy(web, request.url)
        }
    }
    onFullScreenRequested: function(request) { request.accept(); request.toggleOn ? window.showFullScreen() : window.showNormal() }
    onTitleChanged: if (index < tabs.count) tabs.setProperty(index, "label", url.toString() === "about:blank" ? "Nouvel onglet" : title || "Nouvel onglet")
    onIconChanged: if (index < tabs.count) tabs.setProperty(index, "favicon", icon.toString())
    onUrlChanged: {
        browserStore.retainTrackersForOwner(web.profile, web, web.url)
        if (browserStore.origin(web.url.toString()).length === 0) { permissionCoordinator.forgetView(web); controlState.expireContext(web.tabUid); web.policyOrigin = "" };
        if (web === currentView) policyBanner.close();
        if (pendingPermissionView === web) cancelPermission();
        if (index === activeTab) refreshAddress();
        saveTimer.restart()
    }
    onCertificateError: function(error) {
        error.rejectCertificate()
        if (web.upgradedFromHttpUrl.length > 0 && web.upgradeEpoch === web.navigationEpoch && web.url.toString() === web.upgradeExpectedUrl && web === currentView) window.offerHttp(web, web.upgradedFromHttpUrl)
    }
    onPdfPrintingFinished: function(path, success) {
        pdfBusy = false; pdfTarget = null
        showStatus(success ? "PDF enregistré : " + path : "Échec de l’enregistrement PDF.")
        if (smokeMode) smokePdfResult = success ? 1 : -1
    }
    onPermissionRequested: function(permission) { window.askPermission(permission, web) }
    onFindTextFinished: function(result) { if (index === activeTab) findResult = result.activeMatch + " / " + result.numberOfMatches }
    onNewWindowRequested: function(request) {
        const mode = controlState.popupMode(url, tabUid)
        const permitted = mode !== 2 && (request.userInitiated || mode === 1) && /^https?:\/\//i.test(request.requestedUrl.toString())
        controlState.notePopup(tabUid, url, !permitted)
        if (permitted) addTab(request.requestedUrl.toString())
    }
    onLoadingChanged: function(info) {
        if (smokeMode && url.toString().startsWith("file:") && info.status === WebEngineView.LoadSucceededStatus) smokeLocalLoaded = true
        if (info.status === WebEngineView.LoadSucceededStatus) {
            permissionCoordinator.documentLoaded(web, web.navigationEpoch)
            web.upgradedFromHttpUrl = ""
            web.upgradeExpectedUrl = ""
            if (!privateMode) browserStore.recordVisit(url.toString(), title)
        }
        if (info.status === WebEngineView.LoadFailedStatus && index === activeTab) {
            if (web.upgradedFromHttpUrl.length > 0 && web.upgradeEpoch === web.navigationEpoch && info.url.toString() === web.upgradeExpectedUrl) {
                window.offerHttp(web, web.upgradedFromHttpUrl)
                return
            }
            errorText.text = qsTr("Impossible de charger cette page. Vérifiez l’adresse et votre connexion.\n\n") + info.errorString
            errorNotice.open()
        }
    }
    WheelHandler {
        acceptedModifiers: Qt.ControlModifier
        onWheel: function(event) {
            const step = event.angleDelta.y > 0 ? 0.1 : -0.1
            web.zoomFactor = Math.max(0.3, Math.min(3, Math.round((web.zoomFactor + step) * 10) / 10))
            event.accepted = true
        }
    }
    onContextMenuRequested: function(request) {
        request.accepted = true
        pageContextMenu.linkUrl = request.linkUrl.toString()
        pageContextMenu.selectedText = request.selectedText || ""
        pageContextMenu.isEditable = !!request.isContentEditable
        pageContextMenu.popup(web, request.position.x, request.position.y)
    }
}
