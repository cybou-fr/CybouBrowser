import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Dialogs as NativeDialogs
import Cybou

Drawer {
    id: panel
    required property var profile
    property bool privateMode: false
    property var sitePermissions: []
    property string statusText: ""
    signal clearRequested(bool cookies, bool cache, bool history, bool permissions, bool session)
    edge: Qt.RightEdge
    parent: Overlay.overlay
    property real workspaceTop: 0
    property real workspaceHeight: parent.height - workspaceTop
    width: Math.min(520, parent.width - 40)
    y: workspaceTop
    height: workspaceHeight
    modal: false; dim: false
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

    component GlassButton: Button {
        id: gbtn
        scale: down ? 0.95 : hovered ? 1.03 : 1.0
        Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100 } }
        contentItem: Text {
            textFormat: Text.PlainText
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
    function refreshPermissions() { sitePermissions = profile ? profile.listAllPermissions() : [] }
    onOpened: { refreshPermissions(); statusText = "" }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 18
        RowLayout {
            Label { text: qsTr("Paramètres"); color: "#FFFFFF"; font.pixelSize: 26; font.bold: true; Layout.fillWidth: true }
            NavButton { text: "×"; hint: qsTr("Fermer les paramètres"); onClicked: panel.close() }
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ColumnLayout {
                width: parent.width; spacing: 18
                Label { text: qsTr("NAVIGATION"); color: "#38BDF8"; font.letterSpacing: 2; font.pixelSize: 11; font.bold: true }
                Label { text: qsTr("Moteur de recherche"); color: "#E2E8F0"; font.bold: true }
                ComboBox {
                    id: searchProvider
                    Layout.fillWidth: true
                    model: ["DuckDuckGo", "Google", "Bing", "Brave"]
                    currentIndex: model.indexOf(browserStore.searchEngine)
                    onActivated: browserStore.searchEngine = currentText
                    indicator: Text { x: searchProvider.width - width - 14; y: (searchProvider.height - height) / 2; text: "⌄"; color: "#E2E8F0"; font.pixelSize: 20 }
                }
                Switch {
                    id: restoreSwitch
                    text: qsTr("Restaurer les onglets au démarrage")
                    checked: browserStore.restoreSession
                    onToggled: browserStore.restoreSession = checked
                    indicator: Rectangle {
                        implicitWidth: 40; implicitHeight: 24; radius: 12
                        x: restoreSwitch.leftPadding; y: (restoreSwitch.height - height) / 2
                        color: restoreSwitch.checked ? "#0088FF" : "#1E3A5F"
                        Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150 } }
                        Rectangle {
                            x: restoreSwitch.checked ? 19 : 3; y: 3; width: 18; height: 18; radius: 9; color: "white"
                            Behavior on x { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150; easing.type: Easing.OutQuad } }
                        }
                    }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#94A3B8"; text: qsTr("Les fenêtres privées démarrent toujours sur un nouvel onglet."); font.pixelSize: 12 }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#162D4D" }
                CloudSettingsSection { Layout.fillWidth: true }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#162D4D" }
                Label { text: qsTr("APPARENCE & FOND D’ÉCRAN"); color: "#38BDF8"; font.letterSpacing: 2; font.pixelSize: 11; font.bold: true }
                Label { text: qsTr("Fond de la page d’accueil"); color: "#E2E8F0"; font.bold: true }
                ComboBox {
                    id: wallpaperSelector
                    Layout.fillWidth: true
                    model: [qsTr("Alpine Dawn (Recommandé)"), qsTr("Nuit profonde (Dégradé sombre)"), qsTr("Image personnalisée…")]
                    currentIndex: browserStore.wallpaperType === "dark" ? 1 : browserStore.wallpaperType === "custom" ? 2 : 0
                    onActivated: {
                        if (currentIndex === 0) browserStore.wallpaperType = "alpine"
                        else if (currentIndex === 1) browserStore.wallpaperType = "dark"
                        else if (currentIndex === 2) wallpaperFileDialog.open()
                    }
                }
                GlassButton {
                    text: qsTr("Choisir une image de fond…")
                    visible: browserStore.wallpaperType === "custom"
                    onClicked: wallpaperFileDialog.open()
                }
                Label {
                    textFormat: Text.PlainText
                    Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
                    visible: browserStore.wallpaperType === "custom" && browserStore.customWallpaperPath.length > 0
                    text: browserStore.customWallpaperPath; color: "#94A3B8"; font.pixelSize: 11
                }
                Label { text: qsTr("Intensité du verre dépoli (Frosted Glass)"); color: "#E2E8F0"; font.bold: true }
                ComboBox {
                    id: blurSelector
                    Layout.fillWidth: true
                    model: [qsTr("Désactivé (Économie GPU)"), qsTr("Subtil"), qsTr("Intense (Recommandé)")]
                    currentIndex: browserStore.glassBlurLevel
                    onActivated: browserStore.glassBlurLevel = currentIndex
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#162D4D" }
                Label { text: qsTr("TÉLÉCHARGEMENTS"); color: "#38BDF8"; font.letterSpacing: 2; font.pixelSize: 11; font.bold: true }
                Label { textFormat: Text.PlainText; Layout.fillWidth: true; text: browserStore.downloadsPath; wrapMode: Text.WrapAnywhere; color: "#94A3B8"; font.pixelSize: 12 }
                GlassButton { text: qsTr("Choisir le dossier…"); onClicked: downloadFolder.open() }
                CheckBox { text: qsTr("Toujours demander où enregistrer"); checked: browserStore.askDownloadLocation; onToggled: browserStore.askDownloadLocation = checked }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#162D4D" }
                Label { text: qsTr("CONTROL BAR"); color: "#38BDF8"; font.letterSpacing: 2; font.pixelSize: 11; font.bold: true }
                ComboBox {
                    model: ["Français", "Русский"]
                    currentIndex: browserStore.controlBarLanguage === "ru" ? 1 : 0
                    onActivated: browserStore.controlBarLanguage = currentIndex === 1 ? "ru" : "fr"
                    Accessible.name: qsTr("Langue du Control Bar")
                }
                CheckBox { text: qsTr("Réduire les animations"); checked: browserStore.reduceMotion; onToggled: browserStore.reduceMotion = checked }
                CheckBox { text: qsTr("Désactiver les pulsations"); checked: browserStore.disableGlow; onToggled: browserStore.disableGlow = checked }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Les contrôles du site restent visibles à droite de l’adresse et ouvrent des panneaux au survol. Les indicateurs ne prouvent pas qu’un appareil enregistre."); color: "#65758E"; font.pixelSize: 12 }
                Label { text: qsTr("SÉCURITÉ"); color: "#38BDF8"; font.letterSpacing: 2; font.pixelSize: 11; font.bold: true }
                CheckBox { text: qsTr("Mode HTTPS-First (Mise à niveau automatique)"); checked: browserStore.httpsFirst; onToggled: browserStore.httpsFirst = checked }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#94A3B8"; font.pixelSize: 12; text: qsTr("Tente HTTPS pour les adresses HTTP publiques. En cas d’échec, le panneau Réseau peut proposer une exception HTTP explicite et temporaire, sans chiffrement.") }
                CheckBox { text: qsTr("HTTPS par défaut · exceptions HTTP temporaires dans Réseau"); checked: browserStore.httpsOnly; enabled: false }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#94A3B8"; font.pixelSize: 12; text: qsTr("Bloque HTTP et les WebSocket non chiffrés dans tous les profils, y compris sur le réseau local. Une exception HTTP exige une action dans le panneau Réseau, pour une seule navigation dans un profil temporaire de l’onglet. Rechargez les onglets pour interrompre les connexions déjà ouvertes. Les protocoles externes sont désactivés et WebRTC utilise uniquement les interfaces publiques.") }
                CheckBox { text: qsTr("WebRTC strict · aucun UDP direct"); checked: browserStore.strictWebRtc; onToggled: browserStore.strictWebRtc = checked }
                Label {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#94A3B8"; font.pixelSize: 12
                    text: qsTr("Empêche une page de se servir de WebRTC pour sonder le réseau local (paquets STUN vers des adresses locales). Désactive aussi l’UDP direct : les appels vidéo peuvent se dégrader ou échouer sans relais TCP. Désactivé par défaut.")
                          + (browserStore.strictWebRtc !== browserStore.strictWebRtcActive ? " " + qsTr("Redémarrez CYBOU pour appliquer ce changement.") : "")
                }
                Label { text: qsTr("CONFIDENTIALITÉ"); color: "#38BDF8"; font.letterSpacing: 2; font.pixelSize: 11; font.bold: true }
                Switch {
                    id: trackerSwitch
                    text: qsTr("Protection anti-pistage & blocage des traqueurs")
                    checked: browserStore.blockTrackers
                    onToggled: browserStore.blockTrackers = checked
                    indicator: Rectangle {
                        implicitWidth: 40; implicitHeight: 24; radius: 12
                        x: trackerSwitch.leftPadding; y: (trackerSwitch.height - height) / 2
                        color: trackerSwitch.checked ? "#0088FF" : "#1E3A5F"
                        Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150 } }
                        Rectangle {
                            x: trackerSwitch.checked ? 19 : 3; y: 3; width: 18; height: 18; radius: 9; color: "white"
                            Behavior on x { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150; easing.type: Easing.OutQuad } }
                        }
                    }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#94A3B8"; font.pixelSize: 12; text: qsTr("Bloque automatiquement les réseaux publicitaires tiers, les balises de suivi et les outils d’analyse de navigation pour préserver votre vie privée et accélérer le chargement.") }
                Switch {
                    id: cookieSwitch
                    text: qsTr("Bloquer les cookies tiers")
                    checked: browserStore.blockThirdPartyCookies
                    onToggled: browserStore.blockThirdPartyCookies = checked
                    indicator: Rectangle {
                        implicitWidth: 40; implicitHeight: 24; radius: 12
                        x: cookieSwitch.leftPadding; y: (cookieSwitch.height - height) / 2
                        color: cookieSwitch.checked ? "#0088FF" : "#1E3A5F"
                        Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150 } }
                        Rectangle {
                            x: cookieSwitch.checked ? 19 : 3; y: 3; width: 18; height: 18; radius: 9; color: "white"
                            Behavior on x { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150; easing.type: Easing.OutQuad } }
                        }
                    }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#94A3B8"; font.pixelSize: 12; text: qsTr("Appliqué aux profils ouverts. Certains services intégrés peuvent nécessiter ces cookies.") }
                GlassButton { text: qsTr("Effacer les données de navigation…"); enabled: !browserStore.cacheBusy; onClicked: clearDialog.open() }
                Label { textFormat: Text.PlainText; Layout.fillWidth: true; wrapMode: Text.WordWrap; visible: browserStore.cacheBusy || panel.statusText.length > 0; text: browserStore.cacheBusy ? qsTr("Nettoyage du cache en cours…") : panel.statusText; color: "#7DD3FC"; font.pixelSize: 12 }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#162D4D" }
                RowLayout {
                    Label { text: qsTr("AUTORISATIONS DES SITES"); Layout.fillWidth: true; color: "#38BDF8"; font.letterSpacing: 1; font.pixelSize: 11; font.bold: true }
                    NavButton { text: "↻"; hint: qsTr("Actualiser les autorisations"); onClicked: panel.refreshPermissions() }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Les sites demandent votre accord avant d’accéder aux appareils et à la localisation. Réinitialiser un choix fera apparaître une nouvelle demande. Un accès déjà actif cesse à la fermeture de l’onglet."); color: "#94A3B8"; font.pixelSize: 12 }
                Label { visible: panel.sitePermissions.length === 0; text: qsTr("Aucune décision mémorisée"); color: "#64748B" }
                Repeater {
                    model: panel.sitePermissions
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true; implicitHeight: 104; radius: 12
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#70102646" }
                            GradientStop { position: 1.0; color: "#800A1A32" }
                        }
                        border.color: CybouTheme.borderHighlight
                        border.width: 1
                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                            height: 1; color: CybouTheme.glassSheenSubtle; radius: parent.radius
                        }
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 3
                            Label { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.origin.toString(); elide: Text.ElideMiddle; font.bold: true; color: "#FFFFFF" }
                            Label { textFormat: Text.PlainText; Layout.fillWidth: true; text: browserStore.permissionName(modelData.permissionType) + " · " + (modelData.state === 2 ? qsTr("Autorisé") : modelData.state === 3 ? qsTr("Refusé") : qsTr("Demander")); color: modelData.state === 2 ? "#10B981" : modelData.state === 3 ? "#EF4444" : "#F59E0B"; font.pixelSize: 12 }
                            GlassButton { text: qsTr("Réinitialiser"); onClicked: { modelData.reset(); panel.refreshPermissions() } }
                        }
                    }
                }
                Label { textFormat: Text.PlainText; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#64748B"; font.pixelSize: 12; text: panel.privateMode ? qsTr("Profil privé : les données de cette fenêtre restent en mémoire. Elles sont supprimées à sa fermeture. Les téléchargements et favoris restent sur le disque.") : qsTr("Les paramètres et favoris sont partagés entre les fenêtres. Les données privées restent séparées.") }
            }
        }
    }
    NativeDialogs.FolderDialog {
        id: downloadFolder
        title: qsTr("Dossier des téléchargements")
        currentFolder: browserStore.fileUrl(browserStore.downloadsPath)
        onAccepted: if (!browserStore.setDownloadFolder(selectedFolder)) panel.statusText = qsTr("Ce dossier n’est pas accessible en écriture.")
    }
    NativeDialogs.FileDialog {
        id: wallpaperFileDialog
        title: qsTr("Choisir une image de fond")
        nameFilters: [qsTr("Images (*.jpg *.jpeg *.png *.webp *.bmp)")]
        onAccepted: {
            if (selectedFile) {
                if (!browserStore.setCustomWallpaper(selectedFile)) {
                    panel.statusText = qsTr("Format d'image non valide.")
                }
            }
        }
    }
    Dialog {
        id: clearDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 450
        title: panel.privateMode ? qsTr("Effacer les données privées") : qsTr("Effacer les données de navigation")
        modal: true
        header: Label { textFormat: Text.PlainText; text: clearDialog.title; padding: 20; bottomPadding: 8; color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap }
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: Rectangle {
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
        onOpened: {
            historyCheck.checked = !panel.privateMode
            sessionCheck.checked = false
        }
        contentItem: ColumnLayout {
            spacing: 8
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Période : toutes les données du profil de cette fenêtre. Les favoris et fichiers téléchargés sont conservés."); color: CybouTheme.mainText }
            CheckBox { id: cookiesCheck; text: qsTr("Cookies (déconnexion des sites)"); checked: true }
            CheckBox { id: cacheCheck; text: qsTr("Cache des pages"); checked: true }
            CheckBox { id: historyCheck; text: qsTr("Historique de navigation"); enabled: !panel.privateMode }
            CheckBox { id: permissionsCheck; text: qsTr("Décisions d’autorisation"); checked: true }
            CheckBox { id: sessionCheck; text: qsTr("Onglets sauvegardés"); enabled: !panel.privateMode }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#65758E"; font.pixelSize: 12; text: qsTr("L’effacement des cookies ou des onglets sauvegardés ferme les onglets de cette fenêtre. Le stockage local des sites (localStorage, IndexedDB) n’est pas effacé par cette commande.") }
        }
        onAccepted: panel.clearRequested(cookiesCheck.checked, cacheCheck.checked, historyCheck.checked && !panel.privateMode, permissionsCheck.checked, sessionCheck.checked && !panel.privateMode)
    }
}
