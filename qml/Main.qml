import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtWebEngine

ApplicationWindow {
    id: window
    width: 1360; height: 900
    minimumWidth: 820; minimumHeight: 560
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    title: (currentView && currentView.title ? currentView.title + " — " : "") + "CYBOU" + (privateMode ? " — Privé" : "")
    color: "#F5F7FB"
    palette.button: "#EDF1F7"
    palette.buttonText: "#0D2547"
    palette.text: "#0D2547"
    palette.windowText: "#0D2547"
    palette.base: "white"
    palette.highlight: "#D7E4F5"
    palette.highlightedText: "#0D2547"
    palette.placeholderText: "#738197"
    property int activeTab: 0
    readonly property int chromeRowHeight: 36
    readonly property int chromeControlHeight: 28
    property var currentView: views.count > activeTab ? views.itemAt(activeTab) : null
    property bool privateMode: false
    property var closedTabs: []
    property var downloadItems: []
    property string librarySection: "Favoris"
    property bool findVisible: false
    property string findResult: ""
    property bool bookmarked: false
    property bool restoring: true
    readonly property var activeProfile: privateMode ? privateProfile : persistentProfile
    property var pendingPermission: null
    property var pendingPermissionView: null
    function cancelPermission() {
        if (pendingPermission && pendingPermission.isValid) pendingPermission.deny()
        pendingPermission = null
        pendingPermissionView = null
        permissionDialog.close()
    }
    function resolvePermission(allow) {
        if (pendingPermission && pendingPermission.isValid) {
            if (allow && pendingPermissionView === currentView && browserStore.origin(currentView.url.toString()) === browserStore.origin(pendingPermission.origin.toString())) pendingPermission.grant()
            else pendingPermission.deny()
        }
        pendingPermission = null; pendingPermissionView = null; permissionDialog.close()
    }
    function askPermission(permission, view) {
        if (!permission.isValid) return
        if (view !== currentView || pendingPermission || ![1, 2, 3, 6, 7, 8, 9].includes(permission.permissionType)
            || browserStore.origin(view.url.toString()) !== browserStore.origin(permission.origin.toString())) {
            permission.deny(); return
        }
        pendingPermission = permission
        pendingPermissionView = view
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
        }
        if (cookies) browserStore.clearCookies(activeProfile)
        if (cache) browserStore.clearCache(activeProfile)
        settingsPanel.statusText = "Effacement demandé pour les catégories sélectionnées. Les cookies sont supprimés en arrière-plan."
        settingsPanel.refreshPermissions()
    }
    function runPrivateSmokeTest() {
        return privateMode && privateProfile && privateProfile.offTheRecord && persistentProfile === null && tabs.count === 1
    }
    property int smokeStage: 0
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
            library.close()
            currentView.runJavaScript("Notification.requestPermission()")
            smokeStage = 3
            return 0
        }
        if (smokeStage === 3 && pendingPermission) {
            if (pendingPermission.permissionType !== WebEnginePermission.PermissionType.Notifications) return -1
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
            currentView.url = "about:blank"; saveSession()
            settingsPanel.close()
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

    function addTab(url) {
        tabs.append({ initialUrl: url || "about:blank", label: "Nouvel onglet", favicon: "" })
        activeTab = tabs.count - 1
        tabStrip.positionViewAtIndex(activeTab, ListView.Contain)
        saveTimer.restart()
    }
    function closeTab(index) {
        const view = views.itemAt(index)
        if (view === pendingPermissionView) cancelPermission()
        if (!privateMode && view) closedTabs = closedTabs.concat([view.url.toString()])
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
        addTab(url)
    }
    function moveTab(from, to) {
        if (to < 0 || to >= tabs.count) return
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
            if (view) urls.push(view.url.toString())
        }
        browserStore.saveSession(urls)
    }
    function navigate(value) {
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
        download.accept()
        showLibrary("Téléchargements")
    }
    Timer { id: saveTimer; interval: 700; onTriggered: saveSession() }
    onActiveTabChanged: { refreshAddress(); findVisible = false; cancelPermission() }
    onClosing: { cancelPermission(); saveSession() }
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
        const saved = !privateMode && browserStore.restoreSession ? browserStore.session() : []
        for (let i = 0; i < saved.length; ++i) addTab(saved[i])
        if (!tabs.count) addTab("")
        restoring = false
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
    Shortcut { sequence: "Alt+Left"; onActivated: if (currentView) currentView.goBack() }
    Shortcut { sequence: "Alt+Right"; onActivated: if (currentView) currentView.goForward() }
    Shortcut { sequence: "F11"; onActivated: visibility === Window.FullScreen ? showNormal() : showFullScreen() }
    Shortcut { sequence: "Escape"; onActivated: { findVisible = false; if (currentView) { currentView.findText(""); currentView.stop() } } }

    header: ColumnLayout {
        spacing: 0
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: chromeRowHeight; Layout.minimumHeight: chromeRowHeight; Layout.maximumHeight: chromeRowHeight
            color: privateMode ? "#D9DDEC" : "#E5EBF3"
            DragHandler { target: null; onActiveChanged: if (active) window.startSystemMove() }
            TapHandler { onDoubleTapped: window.maximizeToggle() }
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
                        width: Math.max(112, Math.min(200, tabStrip.width / Math.min(tabs.count, 5)))
                        height: chromeControlHeight; radius: 7
                        color: activeTab === index ? "white" : tabMouse.containsMouse ? "#F0F3F8" : "transparent"
                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent; hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                            onClicked: function(mouse) {
                                if (mouse.button === Qt.MiddleButton) closeTab(tabItem.index)
                                else if (mouse.button === Qt.RightButton) tabContext.popup()
                                else activeTab = tabItem.index
                            }
                        }
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 9; anchors.rightMargin: 3; spacing: 6
                            Image { source: tabItem.favicon || "qrc:/assets/cybou.png"; Layout.preferredWidth: 15; Layout.preferredHeight: 15; sourceSize.width: 32; sourceSize.height: 32 }
                            Text { text: tabItem.label; elide: Text.ElideRight; Layout.fillWidth: true; color: "#0D2547"; font.pixelSize: 12 }
                            NavButton { text: "×"; implicitWidth: 22; implicitHeight: 24; hint: "Fermer · Ctrl+W"; onClicked: closeTab(tabItem.index) }
                        }
                        Menu {
                            id: tabContext
                            MenuItem { text: "Déplacer à gauche"; enabled: tabItem.index > 0; onTriggered: moveTab(tabItem.index, tabItem.index - 1) }
                            MenuItem { text: "Déplacer à droite"; enabled: tabItem.index < tabs.count - 1; onTriggered: moveTab(tabItem.index, tabItem.index + 1) }
                            MenuItem { text: "Dupliquer"; onTriggered: addTab(views.itemAt(tabItem.index).url.toString()) }
                            MenuItem { text: "Fermer"; onTriggered: closeTab(tabItem.index) }
                            MenuItem { text: "Rouvrir l’onglet fermé"; enabled: closedTabs.length > 0; onTriggered: reopenTab() }
                        }
                    }
                }
                NavButton { text: "+"; hint: "Nouvel onglet · Ctrl+T"; onClicked: addTab("") }
                Label { visible: privateMode; text: "PRIVÉ"; color: "#526080"; font.pixelSize: 10; padding: 8 }
                Item { Layout.preferredWidth: 18 }
                NavButton { text: "−"; hint: "Réduire"; implicitWidth: 38; implicitHeight: chromeRowHeight; onClicked: window.showMinimized() }
                NavButton { text: window.visibility === Window.Maximized ? "❐" : "□"; hint: "Agrandir / restaurer"; implicitWidth: 38; implicitHeight: chromeRowHeight; onClicked: window.maximizeToggle() }
                Button {
                    id: closeWindow
                    text: "×"; implicitWidth: 38; implicitHeight: chromeRowHeight; padding: 0; hoverEnabled: true
                    Accessible.name: "Fermer CYBOU"
                    background: Rectangle { color: closeWindow.hovered ? "#E11D2E" : "transparent" }
                    contentItem: Text { text: "×"; color: closeWindow.hovered ? "white" : "#0D2547"; font.pixelSize: 20; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: window.close()
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: chromeRowHeight; Layout.minimumHeight: chromeRowHeight; Layout.maximumHeight: chromeRowHeight; color: "white"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; anchors.topMargin: 4; anchors.bottomMargin: 4; spacing: 4
                NavButton { text: "←"; hint: "Retour"; enabled: currentView ? currentView.canGoBack : false; onClicked: currentView.goBack() }
                NavButton { text: "→"; hint: "Suivant"; enabled: currentView ? currentView.canGoForward : false; onClicked: currentView.goForward() }
                NavButton { text: currentView && currentView.loading ? "×" : "↻"; hint: "Actualiser / arrêter"; onClicked: currentView.loading ? currentView.stop() : currentView.reload() }
                NavButton { text: "⌂"; hint: "Accueil"; onClicked: currentView.url = "about:blank" }
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: chromeControlHeight; color: address.activeFocus ? "#FFFFFF" : "#F0F3F8"; radius: 7
                    border.color: address.activeFocus ? "#8097B8" : "#E8EDF4"
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 5; anchors.rightMargin: 4; spacing: 3
                        NavButton { text: "ⓘ"; implicitWidth: 24; hint: "Informations sur la connexion"; onClicked: connectionInfo.open() }
                        TextField {
                            id: address
                            Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 0
                            topPadding: 0; bottomPadding: 0; leftPadding: 4; rightPadding: 4
                            placeholderText: "Rechercher ou saisir une adresse"
                            color: "#0D2547"; font.pixelSize: 13; selectByMouse: true
                            background: Item {}
                            onAccepted: navigate(text)
                            onActiveFocusChanged: if (!activeFocus) refreshAddress()
                        }
                        NavButton { text: bookmarked ? "★" : "☆"; hint: "Favori · Ctrl+D"; onClicked: bookmark() }
                    }
                }
                NavButton { text: "↓"; hint: "Téléchargements · Ctrl+J"; onClicked: showLibrary("Téléchargements") }
                NavButton { id: menuButton; text: "☰"; hint: "Menu"; onClicked: menu.popup(menuButton, 0, menuButton.height) }
                Menu {
                    id: menu
                    MenuItem { text: "Nouvel onglet                         Ctrl+T"; onTriggered: addTab("") }
                    MenuItem { text: "Rouvrir l’onglet fermé          Ctrl+Shift+T"; enabled: closedTabs.length > 0; onTriggered: reopenTab() }
                    MenuItem { text: "Nouvelle fenêtre privée     Ctrl+Shift+N"; onTriggered: browserStore.newPrivateWindow() }
                    MenuSeparator {}
                    MenuItem { text: "Favoris"; onTriggered: showLibrary("Favoris") }
                    MenuItem { text: "Historique                                Ctrl+H"; onTriggered: showLibrary("Historique") }
                    MenuItem { text: "Téléchargements                      Ctrl+J"; onTriggered: showLibrary("Téléchargements") }
                    MenuItem { text: "Rechercher dans la page           Ctrl+F"; onTriggered: { findVisible = true; findInput.forceActiveFocus() } }
                    MenuSeparator {}
                    MenuItem { text: "Zoom +"; onTriggered: currentView.zoomFactor = Math.min(3, currentView.zoomFactor + 0.1) }
                    MenuItem { text: "Zoom −"; onTriggered: currentView.zoomFactor = Math.max(0.3, currentView.zoomFactor - 0.1) }
                    MenuItem { text: "Zoom 100 %"; onTriggered: currentView.zoomFactor = 1 }
                    MenuItem { text: "Plein écran                                  F11"; onTriggered: window.visibility === Window.FullScreen ? window.showNormal() : window.showFullScreen() }
                    MenuSeparator {}
                    MenuItem { text: "Paramètres                                     Ctrl+,"; onTriggered: settingsPanel.open() }
                    MenuItem { text: "À propos de CYBOU"; onTriggered: about.open() }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 2; color: "#E5EAF1"
            Rectangle { height: 2; width: parent.width * (currentView ? currentView.loadProgress / 100 : 0); color: "#E11D2E"; visible: currentView ? currentView.loading : false }
        }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: findVisible ? 48 : 0; visible: findVisible; color: "white"
            RowLayout {
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 8
                TextField { id: findInput; placeholderText: "Rechercher dans la page"; onTextChanged: if (currentView) currentView.findText(text); onAccepted: if (currentView) currentView.findText(text) }
                Label { text: findResult; color: "#65758E" }
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
            anchors.fill: parent
            visible: activeTab === index && url.toString() !== "about:blank"
            url: initialUrl
            profile: privateMode ? privateProfile : persistentProfile
            settings.javascriptCanOpenWindows: true
            settings.localContentCanAccessRemoteUrls: false
            settings.localContentCanAccessFileUrls: false
            settings.allowRunningInsecureContent: false
            settings.fullScreenSupportEnabled: true
            onFullScreenRequested: function(request) { request.accept(); request.toggleOn ? window.showFullScreen() : window.showNormal() }
            onTitleChanged: if (index < tabs.count) tabs.setProperty(index, "label", url.toString() === "about:blank" ? "Nouvel onglet" : title || "Nouvel onglet")
            onIconChanged: if (index < tabs.count) tabs.setProperty(index, "favicon", icon.toString())
            onUrlChanged: { if (pendingPermissionView === web) cancelPermission(); if (index === activeTab) refreshAddress(); saveTimer.restart() }
            onCertificateError: function(error) { error.rejectCertificate() }
            onPermissionRequested: function(permission) { window.askPermission(permission, web) }
            onFindTextFinished: function(result) { if (index === activeTab) findResult = result.activeMatch + " / " + result.numberOfMatches }
            onNewWindowRequested: function(request) {
                if (request.userInitiated && /^https?:\/\//i.test(request.requestedUrl.toString())) addTab(request.requestedUrl.toString())
            }
            onLoadingChanged: function(info) {
                if (info.status === WebEngineView.LoadSucceededStatus && !privateMode) browserStore.recordVisit(url.toString(), title)
                if (info.status === WebEngineView.LoadFailedStatus && index === activeTab) {
                    errorText.text = "Impossible de charger cette page. Vérifiez l’adresse et votre connexion.\n\n" + info.errorString
                    errorNotice.open()
                }
            }
        }
    }
    Connections { target: currentView; function onUrlChanged() { window.refreshAddress() } }
    Rectangle {
        anchors.fill: parent; visible: currentView ? currentView.url.toString() === "about:blank" : true; color: "#F5F7FB"
        ColumnLayout {
            anchors.centerIn: parent; anchors.verticalCenterOffset: -35
            width: Math.min(640, parent.width - 80); spacing: 22
            Image { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 108; Layout.preferredHeight: 108; source: "qrc:/assets/cybou.png"; sourceSize.width: 256; sourceSize.height: 256 }
            Text { Layout.alignment: Qt.AlignHCenter; text: "CYBOU"; color: "#0D2547"; font { pixelSize: 46; bold: true; letterSpacing: 8 } }
            Text { Layout.alignment: Qt.AlignHCenter; text: privateMode ? "VOTRE ESPACE PRIVÉ" : "NAVIGUEZ EN TOUTE SIMPLICITÉ"; color: "#65758E"; font { pixelSize: 11; letterSpacing: 3 } }
            Rectangle { Layout.alignment: Qt.AlignHCenter; implicitWidth: 96; implicitHeight: 3; color: "white"
                Rectangle { width: 32; height: 3; color: "#0D2547" }
                Rectangle { x: 64; width: 32; height: 3; color: "#E11D2E" }
            }
            TextField {
                Layout.fillWidth: true; Layout.topMargin: 10; implicitHeight: 58
                leftPadding: 24; rightPadding: 24; placeholderText: "Où souhaitez-vous aller ?"
                font.pixelSize: 16; color: "#0D2547"; selectByMouse: true
                background: Rectangle { radius: 18; color: "white"; border.color: "#E1E7EF" }
                onAccepted: { navigate(text); text = "" }
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter; spacing: 12
                Button { text: "Favoris"; onClicked: showLibrary("Favoris") }
                Button { text: "Historique"; visible: !privateMode; onClicked: showLibrary("Historique") }
            }
            Text { Layout.alignment: Qt.AlignHCenter; text: privateMode ? "Les visites privées ne sont pas enregistrées dans l’historique." : "Un nouvel horizon. Un espace à vous."; color: "#8491A5"; font.pixelSize: 12 }
        }
        Text { anchors.bottom: parent.bottom; anchors.bottomMargin: 24; anchors.horizontalCenter: parent.horizontalCenter; text: "CYBOU  /  ÉDITION 0.3"; color: "#8A97AA"; font { pixelSize: 10; letterSpacing: 2 } }
    }
    Drawer {
        id: library
        edge: Qt.RightEdge; width: Math.min(460, window.width - 60); height: window.height
        background: Rectangle { color: "#F5F7FB"; border.color: "#DFE5ED" }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 14
            RowLayout {
                Label { text: librarySection; font.pixelSize: 23; font.bold: true; color: "#0D2547"; Layout.fillWidth: true }
                NavButton { text: "×"; hint: "Fermer"; onClicked: library.close() }
            }
            RowLayout {
                Button { text: "Favoris"; onClicked: librarySection = "Favoris" }
                Button { text: "Historique"; onClicked: librarySection = "Historique" }
                Button { text: "Fichiers"; onClicked: librarySection = "Téléchargements" }
            }
            TextField { id: librarySearch; Layout.fillWidth: true; placeholderText: "Filtrer…"; visible: librarySection !== "Téléchargements" }
            Button { text: "Effacer l’historique"; visible: librarySection === "Historique"; onClicked: clearHistoryDialog.open() }
            Button { text: "Ouvrir le dossier Téléchargements"; visible: librarySection === "Téléchargements"; onClicked: browserStore.openDownloads() }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 8
                model: librarySection === "Favoris" ? browserStore.bookmarks : librarySection === "Historique" ? browserStore.history : downloadItems
                delegate: Rectangle {
                    id: entry
                    required property var modelData
                    property bool download: librarySection === "Téléchargements"
                    property bool matches: download || (modelData.title + " " + modelData.url).toLowerCase().includes(librarySearch.text.toLowerCase())
                    width: ListView.view.width; height: matches ? (download ? 112 : 82) : 0; visible: matches
                    radius: 12; color: "white"; border.color: "#E1E7EF"
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 12; spacing: 4
                        Label { Layout.fillWidth: true; text: entry.download ? entry.modelData.downloadFileName : entry.modelData.title; color: "#0D2547"; elide: Text.ElideRight; font.bold: true }
                        Label { Layout.fillWidth: true; text: entry.download ? (entry.modelData.state === WebEngineDownloadRequest.DownloadCompleted ? "Terminé" : entry.modelData.isFinished ? "Interrompu / annulé" : Math.round(entry.modelData.receivedBytes / 1024) + " Ko reçus") : entry.modelData.url; elide: Text.ElideRight; color: "#65758E"; font.pixelSize: 11 }
                        ProgressBar { visible: entry.download; Layout.fillWidth: true; from: 0; to: entry.download ? Math.max(1, entry.modelData.totalBytes) : 1; value: entry.download ? entry.modelData.receivedBytes : 0; indeterminate: entry.download && entry.modelData.totalBytes <= 0 && !entry.modelData.isFinished }
                        RowLayout {
                            Button { text: entry.download ? "Annuler" : "Ouvrir"; enabled: !entry.download || !entry.modelData.isFinished; onClicked: { if (entry.download) entry.modelData.cancel(); else { addTab(entry.modelData.url); library.close() } } }
                            Button { text: "Retirer"; visible: librarySection === "Favoris"; onClicked: browserStore.toggleBookmark(entry.modelData.url, entry.modelData.title) }
                        }
                    }
                }
                Label { anchors.centerIn: parent; visible: parent.count === 0; text: "Rien ici pour le moment"; color: "#8491A5" }
            }
        }
    }
    Dialog {
        id: permissionDialog
        anchors.centerIn: parent; width: Math.min(460, window.width - 40)
        title: "Autorisation du site"; modal: true
        closePolicy: Popup.CloseOnEscape
        contentItem: ColumnLayout {
            spacing: 14
            Label { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; font.bold: true; text: pendingPermission ? pendingPermission.origin.toString() : "" }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: pendingPermission ? "Ce site demande : " + browserStore.permissionName(pendingPermission.permissionType) : "" }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#65758E"; font.pixelSize: 12; text: privateMode ? "Décision limitée à cette fenêtre privée." : "Les décisions persistantes sont révocables dans Paramètres → Autorisations des sites." }
        }
        footer: DialogButtonBox {
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
        Label { text: "Toutes les visites enregistrées seront supprimées." }
        onAccepted: browserStore.clearHistory()
    }
    Dialog {
        id: connectionInfo; anchors.centerIn: parent; width: 400; title: "Connexion"; modal: true; standardButtons: Dialog.Ok
        Label { width: parent.width; wrapMode: Text.WordWrap; text: currentView && currentView.url.toString().startsWith("https://") ? "Cette page utilise HTTPS. Le chiffrement ne garantit pas la fiabilité du site." : "Cette page n’utilise pas HTTPS ou affiche la page d’accueil locale." }
    }
    Dialog {
        id: about; anchors.centerIn: parent; width: 440; title: "CYBOU 0.3"; modal: true; standardButtons: Dialog.Ok
        Label { width: parent.width; wrapMode: Text.WordWrap; text: "Navigateur Qt Quick + Qt WebEngine.\n\nVersion de développement. Les sites demandent votre accord pour les permissions prises en charge. Le mode privé n’anonymise pas le trafic réseau. Les fichiers téléchargés restent sur le disque.\n\nCtrl+T : nouvel onglet · Ctrl+L : adresse\nCtrl+D : favori · Ctrl+F : recherche\nCtrl+H : historique · Ctrl+J : téléchargements\nCtrl+, : paramètres · Ctrl+Shift+N : fenêtre privée" }
    }
    Popup {
        id: errorNotice; anchors.centerIn: parent; width: Math.min(520, window.width - 40); padding: 24; modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: "white"; radius: 16; border.color: "#DFE5ED" }
        contentItem: Label { id: errorText; wrapMode: Text.WordWrap; color: "#0D2547" }
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


