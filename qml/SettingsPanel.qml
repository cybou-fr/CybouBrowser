import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as NativeDialogs

Drawer {
    id: panel
    required property var profile
    property bool privateMode: false
    property var sitePermissions: []
    property string statusText: ""
    signal clearRequested(bool cookies, bool cache, bool history, bool permissions, bool session)
    edge: Qt.RightEdge
    width: Math.min(520, parent.width - 40)
    height: parent.height
    background: Rectangle { color: "#F5F7FB"; border.color: "#DFE5ED" }
    function refreshPermissions() { sitePermissions = profile ? profile.listAllPermissions() : [] }
    onOpened: { refreshPermissions(); statusText = "" }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 18
        RowLayout {
            Label { text: "Paramètres"; color: "#0D2547"; font.pixelSize: 26; font.bold: true; Layout.fillWidth: true }
            NavButton { text: "×"; hint: "Fermer les paramètres"; onClicked: panel.close() }
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ColumnLayout {
                width: parent.width; spacing: 18
                Label { text: "NAVIGATION"; color: "#65758E"; font.letterSpacing: 2; font.pixelSize: 11 }
                Label { text: "Moteur de recherche"; font.bold: true }
                ComboBox {
                    id: searchProvider
                    Layout.fillWidth: true
                    model: ["DuckDuckGo", "Google", "Bing", "Brave"]
                    currentIndex: model.indexOf(browserStore.searchEngine)
                    onActivated: browserStore.searchEngine = currentText
                    indicator: Text { x: searchProvider.width - width - 14; y: (searchProvider.height - height) / 2; text: "⌄"; color: "#0D2547"; font.pixelSize: 20 }
                }
                Switch {
                    id: restoreSwitch
                    text: "Restaurer les onglets au démarrage"
                    checked: browserStore.restoreSession
                    onToggled: browserStore.restoreSession = checked
                    indicator: Rectangle {
                        implicitWidth: 40; implicitHeight: 24; radius: 12
                        x: restoreSwitch.leftPadding; y: (restoreSwitch.height - height) / 2
                        color: restoreSwitch.checked ? "#0D2547" : "#C7D1DF"
                        Rectangle { x: restoreSwitch.checked ? 19 : 3; y: 3; width: 18; height: 18; radius: 9; color: "white" }
                    }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#65758E"; text: "Les fenêtres privées démarrent toujours sur un nouvel onglet."; font.pixelSize: 12 }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#DFE5ED" }
                Label { text: "TÉLÉCHARGEMENTS"; color: "#65758E"; font.letterSpacing: 2; font.pixelSize: 11 }
                Label { Layout.fillWidth: true; text: browserStore.downloadsPath; wrapMode: Text.WrapAnywhere; color: "#65758E"; font.pixelSize: 12 }
                Button { text: "Choisir le dossier…"; onClicked: downloadFolder.open() }
                CheckBox { text: "Toujours demander où enregistrer"; checked: browserStore.askDownloadLocation; onToggled: browserStore.askDownloadLocation = checked }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#DFE5ED" }
                Label { text: qsTr("CONTROL BAR"); color: "#65758E"; font.letterSpacing: 2; font.pixelSize: 11 }
                ComboBox {
                    model: ["Français", "Русский"]
                    currentIndex: browserStore.controlBarLanguage === "ru" ? 1 : 0
                    onActivated: browserStore.controlBarLanguage = currentIndex === 1 ? "ru" : "fr"
                    Accessible.name: qsTr("Langue du Control Bar")
                }
                CheckBox { text: qsTr("Réduire les animations"); checked: browserStore.reduceMotion; onToggled: browserStore.reduceMotion = checked }
                CheckBox { text: qsTr("Désactiver les pulsations"); checked: browserStore.disableGlow; onToggled: browserStore.disableGlow = checked }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Les contrôles du site sont intégrés à la navigation et masqués pendant la saisie de l’adresse. Les indicateurs ne prouvent pas qu’un appareil enregistre."); color: "#65758E"; font.pixelSize: 12 }
                Label { text: qsTr("SÉCURITÉ"); color: "#65758E"; font.letterSpacing: 2; font.pixelSize: 11 }
                CheckBox { text: qsTr("Connexions HTTPS uniquement"); checked: browserStore.httpsOnly; onToggled: browserStore.httpsOnly = checked }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#65758E"; font.pixelSize: 12; text: qsTr("Bloque HTTP et les WebSocket non chiffrés dans tous les profils, y compris sur le réseau local. Aucun repli HTTP. Rechargez les onglets pour interrompre les connexions déjà ouvertes. Les protocoles externes sont désactivés et WebRTC utilise uniquement les interfaces publiques.") }
                Label { text: "CONFIDENTIALITÉ"; color: "#65758E"; font.letterSpacing: 2; font.pixelSize: 11 }
                Switch {
                    id: cookieSwitch
                    text: "Bloquer les cookies tiers"
                    checked: browserStore.blockThirdPartyCookies
                    onToggled: browserStore.blockThirdPartyCookies = checked
                    indicator: Rectangle {
                        implicitWidth: 40; implicitHeight: 24; radius: 12
                        x: cookieSwitch.leftPadding; y: (cookieSwitch.height - height) / 2
                        color: cookieSwitch.checked ? "#0D2547" : "#C7D1DF"
                        Rectangle { x: cookieSwitch.checked ? 19 : 3; y: 3; width: 18; height: 18; radius: 9; color: "white" }
                    }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#65758E"; font.pixelSize: 12; text: "Appliqué aux profils ouverts. Certains services intégrés peuvent nécessiter ces cookies. Le blocage ne remplace pas un bloqueur de traqueurs." }
                Button { text: "Effacer les données de navigation…"; enabled: !browserStore.cacheBusy; onClicked: clearDialog.open() }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; visible: browserStore.cacheBusy || panel.statusText.length > 0; text: browserStore.cacheBusy ? "Nettoyage du cache en cours…" : panel.statusText; color: "#526887"; font.pixelSize: 12 }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#DFE5ED" }
                RowLayout {
                    Label { text: "AUTORISATIONS DES SITES"; Layout.fillWidth: true; color: "#65758E"; font.letterSpacing: 1; font.pixelSize: 11 }
                    NavButton { text: "↻"; hint: "Actualiser les autorisations"; onClicked: panel.refreshPermissions() }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: "Les sites demandent votre accord avant d’accéder aux appareils et à la localisation. Réinitialiser un choix fera apparaître une nouvelle demande. Un accès déjà actif cesse à la fermeture de l’onglet."; color: "#65758E"; font.pixelSize: 12 }
                Label { visible: panel.sitePermissions.length === 0; text: "Aucune décision mémorisée"; color: "#8491A5" }
                Repeater {
                    model: panel.sitePermissions
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true; implicitHeight: 104; radius: 12; color: "white"; border.color: "#E1E7EF"
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 3
                            Label { Layout.fillWidth: true; text: modelData.origin.toString(); elide: Text.ElideMiddle; font.bold: true }
                            Label { Layout.fillWidth: true; text: browserStore.permissionName(modelData.permissionType) + " · " + (modelData.state === 2 ? "Autorisé" : modelData.state === 3 ? "Refusé" : "Demander"); color: "#65758E"; font.pixelSize: 12 }
                            Button { text: "Réinitialiser"; onClicked: { modelData.reset(); panel.refreshPermissions() } }
                        }
                    }
                }
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#65758E"; font.pixelSize: 12; text: panel.privateMode ? "Profil privé : les données de cette fenêtre restent en mémoire. Elles sont supprimées à sa fermeture. Les téléchargements et favoris restent sur le disque." : "Les paramètres et favoris sont partagés entre les fenêtres. Les données privées restent séparées." }
            }
        }
    }
    NativeDialogs.FolderDialog {
        id: downloadFolder
        title: "Dossier des téléchargements"
        currentFolder: browserStore.fileUrl(browserStore.downloadsPath)
        onAccepted: if (!browserStore.setDownloadFolder(selectedFolder)) panel.statusText = "Ce dossier n’est pas accessible en écriture."
    }
    Dialog {
        id: clearDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(470, parent.width - 40)
        title: panel.privateMode ? "Effacer les données privées" : "Effacer les données de navigation"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: {
            historyCheck.checked = !panel.privateMode
            sessionCheck.checked = false
        }
        contentItem: ColumnLayout {
            spacing: 8
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: "Période : toutes les données du profil de cette fenêtre. Les favoris et fichiers téléchargés sont conservés." }
            CheckBox { id: cookiesCheck; text: "Cookies (déconnexion des sites)"; checked: true }
            CheckBox { id: cacheCheck; text: "Cache des pages"; checked: true }
            CheckBox { id: historyCheck; text: "Historique de navigation"; enabled: !panel.privateMode }
            CheckBox { id: permissionsCheck; text: "Décisions d’autorisation"; checked: true }
            CheckBox { id: sessionCheck; text: "Onglets sauvegardés"; enabled: !panel.privateMode }
            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#65758E"; font.pixelSize: 12; text: "L’effacement des cookies ou des onglets sauvegardés ferme les onglets de cette fenêtre. Le stockage local des sites (localStorage, IndexedDB) n’est pas effacé par cette commande." }
        }
        onAccepted: panel.clearRequested(cookiesCheck.checked, cacheCheck.checked, historyCheck.checked && !panel.privateMode, permissionsCheck.checked, sessionCheck.checked && !panel.privateMode)
    }
}
