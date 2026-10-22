import QtQuick
import QtWebEngine
import Cybou

WebEngineView {
    id: web
    required property int index
    required property string initialUrl
    required property bool isMuted
    required property string tabUid

    property var temporaryProfile: null
    property var defaultProfile: null
    property bool isCurrentTab: false
    property bool isPrivate: false
    property bool smokeMode: false

    property var store: null
    property var coordinator: null
    property var controller: null
    property var pendingPermissionView: null

    signal askPermissionRequested(var permission, var view)
    signal offerHttpRequested(var view, var url)
    signal endTemporaryRequested(var view, string url)
    signal releaseRequested(string uid)
    signal downloadRequested(var download)
    signal applyPolicyRequested(var view, var url)
    signal newTabRequested(string url)
    signal fullScreenToggled(bool fullScreen)
    signal tabPropertyUpdate(int idx, string prop, var val)
    signal addressRefresh()
    signal statusNotice(string msg)
    signal errorNotice(string msg)
    signal contextMenuOpen(var targetView, string link, string text, bool editable, int x, int y)
    signal dismissControlBar()
    signal closePolicyBanner()
    signal restartSaveTimer()
    signal cancelPermissionRequested()
    signal findResultUpdated(string result)
    signal pdfFinished(string path, bool success)
    signal localLoaded()
    signal restrictedModeTriggered(var view, string reason)

    property bool restrictedMode: false
    property bool restrictedModeOverridden: false
    readonly property bool restrictedActive: restrictedMode && !restrictedModeOverridden
    property string restrictedReason: ""
    property string blockedHttpUrl: ""
    property int blockedHttpEpoch: -1
    readonly property bool temporaryContext: !!temporaryProfile
    readonly property bool temporaryTransport: store ? store.isTemporaryHttpProfile(temporaryProfile || null) : false
    property string policyOrigin: initialUrl !== "about:blank" && store ? store.origin(initialUrl) : ""
    property int navigationEpoch: 0
    readonly property var grantedKeys: {
        if (coordinator) coordinator.revision
        return coordinator ? coordinator.grantedKeys(web, navigationEpoch) : []
    }
    readonly property bool hasPermissionLease: grantedKeys.length > 0
    Component.onDestruction: {
        if (coordinator) coordinator.forgetView(web)
        if (controller) controller.expireContext(tabUid)
        if (store) store.releaseTrackerOwner(web.profile, web)
        releaseRequested(tabUid)
    }
    property bool policyNeedsReload: false
    property string upgradedFromHttpUrl: ""
    property string upgradeExpectedUrl: ""
    property int upgradeEpoch: -1
    anchors.fill: parent
    visible: isCurrentTab && url.toString() !== "about:blank"
    url: initialUrl
    profile: temporaryProfile || defaultProfile
    Connections {
        target: web.temporaryContext ? web.profile : null
        function onDownloadRequested(download) { web.downloadRequested(download) }
    }
    audioMuted: isMuted || restrictedActive
    onRecentlyAudibleChanged: function(value) { tabPropertyUpdate(index, "audible", value) }
    settings.javascriptEnabled: !restrictedActive && (controller ? controller.settingFor(url, "javascript", tabUid) : true)
    settings.localStorageEnabled: !restrictedActive && (controller ? controller.settingFor(url, "storage", tabUid) : true)
    settings.javascriptCanOpenWindows: !restrictedActive && (controller ? controller.popupMode(url, tabUid) !== 2 : true)
    settings.localContentCanAccessRemoteUrls: false
    settings.localContentCanAccessFileUrls: false
    settings.allowRunningInsecureContent: false
    settings.unknownUrlSchemePolicy: WebEngineSettings.DisallowUnknownUrlSchemes
    settings.webRTCPublicInterfacesOnly: true
    settings.fullScreenSupportEnabled: true

    Component.onCompleted: {
        if (store && initialUrl !== "about:blank" && store.requiresRestrictedBrowsing(initialUrl)) {
            web.restrictedMode = true
            web.restrictedReason = store.restrictedBrowsingReason(initialUrl)
            if (isCurrentTab && web.restrictedActive) restrictedModeTriggered(web, web.restrictedReason)
        }
    }

    property double lastExternalNoticeAt: 0
    function externalLinkMessage(info) {
        const target = info.target || ""
        if (info.kind === "mail")
            return target.length ? qsTr("Les liens « mailto: » ne s’ouvrent pas : les applications externes sont désactivées. Adresse : %1 — clic droit › Copier l’adresse du lien.").arg(target)
                                 : qsTr("Les liens « mailto: » ne s’ouvrent pas : les applications externes sont désactivées. Clic droit › Copier l’adresse du lien.")
        if (info.kind === "phone")
            return target.length ? qsTr("Les liens téléphoniques ne s’ouvrent pas : les applications externes sont désactivées. Numéro : %1 — clic droit › Copier l’adresse du lien.").arg(target)
                                 : qsTr("Les liens téléphoniques ne s’ouvrent pas : les applications externes sont désactivées. Clic droit › Copier l’adresse du lien.")
        return qsTr("Ce lien ouvrirait une application externe (« %1: »). Les applications externes sont désactivées dans CYBOU.").arg(info.scheme)
    }
    // With DisallowUnknownUrlSchemes the engine refuses a link to an external application before any
    // navigation hook runs (measured by --protocol-test), so the hint is driven by the link under the
    // pointer when the left button goes down (Main.qml observes the press) instead of by a navigation
    // request. The view filters mouse events for its own children, so the observer cannot live inside it.
    property string hoveredLink: ""
    onLinkHovered: function(hoveredUrl) { web.hoveredLink = hoveredUrl.toString() }
    function noteLinkPress() {
        if (!store || !isCurrentTab || !hoveredLink.length) return
        const info = store.externalLinkInfo(hoveredLink)
        const now = Date.now()
        if (!info.kind || now - lastExternalNoticeAt < 1500) return
        lastExternalNoticeAt = now
        statusNotice(externalLinkMessage(info))
    }
    onNavigationRequested: function(request) {
        if (request.isMainFrame) {
            web.navigationEpoch++
            const nextOrigin = store ? store.origin(request.url.toString()) : ""
            if (web.policyOrigin !== nextOrigin) {
                web.restrictedModeOverridden = false
            }
            if (store && store.requiresRestrictedBrowsing(request.url)) {
                web.restrictedMode = true
                web.restrictedReason = store.restrictedBrowsingReason(request.url)
                if (isCurrentTab && !web.restrictedModeOverridden) restrictedModeTriggered(web, web.restrictedReason)
            } else {
                web.restrictedMode = false
                web.restrictedReason = ""
                web.restrictedModeOverridden = false
            }
            if (request.url.toString() !== web.upgradeExpectedUrl) {
                web.upgradedFromHttpUrl = ""
                web.upgradeExpectedUrl = ""
                web.upgradeEpoch = -1
            } else web.upgradeEpoch = web.navigationEpoch
            web.blockedHttpUrl = ""
            web.blockedHttpEpoch = -1
            if (isCurrentTab) { closePolicyBanner(); dismissControlBar() }
            if (pendingPermissionView === web) cancelPermissionRequested()
            if (web.temporaryTransport && store && !store.canStartTemporaryHttp(web.profile, request.url)) {
                request.reject()
                endTemporaryRequested(web, request.url.toString())
                return
            }
            if (!web.temporaryTransport && store && store.shouldUpgradeToHttps(request.url)) {
                request.reject()
                web.upgradedFromHttpUrl = request.url.toString()
                web.upgradeExpectedUrl = store.upgradeToHttps(request.url).toString()
                web.url = web.upgradeExpectedUrl
                return
            }
        }
        const reason = store ? store.navigationBlockReason(request.url, web.profile, request.isMainFrame) : ""
        if (reason.length > 0) {
            request.reject()
            if (request.isMainFrame && isCurrentTab) {
                if (store && store.canRequestTemporaryHttp(request.url)) offerHttpRequested(web, request.url)
                else errorNotice(reason)
            }
            return
        }
        if (request.isMainFrame) {
            const nextOrigin = store ? store.origin(request.url.toString()) : ""
            if (web.policyOrigin !== nextOrigin && controller) controller.expireContext(web.tabUid)
            web.policyOrigin = nextOrigin
            applyPolicyRequested(web, request.url)
        }
    }
    onFullScreenRequested: function(request) { request.accept(); fullScreenToggled(request.toggleOn) }
    onTitleChanged: tabPropertyUpdate(index, "label", url.toString() === "about:blank" ? qsTr("Nouvel onglet") : title || qsTr("Nouvel onglet"))
    onIconChanged: tabPropertyUpdate(index, "favicon", icon.toString())
    onUrlChanged: {
        if (store) store.retainTrackersForOwner(web.profile, web, web.url)
        if (store && store.origin(web.url.toString()).length === 0) {
            if (coordinator) coordinator.forgetView(web)
            if (controller) controller.expireContext(web.tabUid)
            web.policyOrigin = ""
        }
        if (isCurrentTab) { closePolicyBanner(); addressRefresh() }
        restartSaveTimer()
    }
    onCertificateError: function(error) {
        error.rejectCertificate()
        if (web.upgradedFromHttpUrl.length > 0 && web.upgradeEpoch === web.navigationEpoch && web.url.toString() === web.upgradeExpectedUrl && isCurrentTab) offerHttpRequested(web, web.upgradedFromHttpUrl)
    }
    onPdfPrintingFinished: function(filePath, success) {
        pdfFinished(filePath, success)
    }
    onPermissionRequested: function(permission) {
        if (web.restrictedActive) {
            permission.deny()
            return
        }
        askPermissionRequested(permission, web)
    }
    // Screen capture is permanently refused (EffectivePermissionPolicy). getDisplayMedia arrives here, not as a
    // permission request: cancel it and record the attempt so the Screen control shows it.
    onDesktopMediaRequested: function(request) {
        request.cancel()
        if (controller) controller.noteBlockedRequest("screen", tabUid, url)
    }
    onFindTextFinished: function(result) { if (isCurrentTab) findResultUpdated(result.activeMatch + " / " + result.numberOfMatches) }
    onNewWindowRequested: function(request) {
        const mode = controller ? controller.popupMode(url, tabUid) : 0
        const permitted = mode !== 2 && (request.userInitiated || mode === 1) && /^https?:\/\//i.test(request.requestedUrl.toString())
        if (controller) controller.notePopup(tabUid, url, !permitted)
        if (permitted) newTabRequested(request.requestedUrl.toString())
    }
    onIsCurrentTabChanged: {
        if (isCurrentTab && web.restrictedActive && url.toString() !== "about:blank") {
            restrictedModeTriggered(web, web.restrictedReason)
        }
    }
    property double lastRenderCrashAt: 0
    // A dead renderer leaves a blank tab. Reload once; a second crash within 15 s is reported instead of looping.
    onRenderProcessTerminated: function(status, exitCode) {
        if (status === WebEngineView.NormalTerminationStatus) return
        const now = Date.now()
        const repeated = now - lastRenderCrashAt < 15000
        lastRenderCrashAt = now
        if (repeated) {
            if (isCurrentTab) errorNotice(qsTr("Cette page a planté à nouveau. Elle n’a pas été rechargée automatiquement."))
            return
        }
        if (isCurrentTab) statusNotice(qsTr("La page a planté ; rechargement en cours."))
        reload()
    }
    onLoadingChanged: function(info) {
        if (smokeMode && url.toString().startsWith("file:") && info.status === WebEngineView.LoadSucceededStatus) localLoaded()
        if (info.status === WebEngineView.LoadSucceededStatus) {
            if (coordinator) coordinator.documentLoaded(web, web.navigationEpoch)
            web.upgradedFromHttpUrl = ""
            web.upgradeExpectedUrl = ""
            if (!isPrivate && !web.restrictedActive && store) store.recordVisit(url.toString(), title)
            if (web.restrictedActive && isCurrentTab) {
                restrictedModeTriggered(web, web.restrictedReason)
            }
        }
        if (info.status === WebEngineView.LoadFailedStatus && isCurrentTab) {
            // Downloads and cancelled navigations are expected lifecycle events.
            if (info.isDownload || info.errorCode === -3) return
            if (web.upgradedFromHttpUrl.length > 0 && web.upgradeEpoch === web.navigationEpoch && info.url.toString() === web.upgradeExpectedUrl) {
                offerHttpRequested(web, web.upgradedFromHttpUrl)
                return
            }
            errorNotice(qsTr("Impossible de charger cette page. Vérifiez l’adresse et votre connexion.\n\n") + info.errorString)
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
        contextMenuOpen(web, request.linkUrl.toString(), request.selectedText || "", !!request.isContentEditable, request.position.x, request.position.y)
    }
}
