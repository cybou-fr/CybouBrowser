import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: card
    property var capability: ({})
    property bool pinned: false
    property bool pointerInside: hover.hovered
    signal policyRequested(string key, int mode)
    signal permissionRequested(string key, int mode)
    signal decisionRequested(bool allow)
    signal reloadRequested()
    signal cookieRequested()
    width: 330; padding: 16
    height: Math.min(implicitHeight, parent.height - 16)
    focus: pinned; modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { color: "#F0203550"; radius: 15; border.color: "#70B6CDE5" }
    contentItem: ScrollView {
        implicitHeight: content.implicitHeight
        contentWidth: availableWidth
        clip: true
        HoverHandler { id: hover }
        ColumnLayout {
        id: content
        width: parent.width
        spacing: 10
        RowLayout {
            Label { text: card.capability.title || ""; color: "white"; font.bold: true; font.pixelSize: 15; Layout.fillWidth: true }
            Button { text: "×"; implicitWidth: 28; implicitHeight: 28; onClicked: card.close(); Accessible.name: qsTr("Fermer les détails") }
        }
        Label { Layout.fillWidth: true; text: card.capability.scope || ""; color: "#A9C1DC"; font.pixelSize: 11; wrapMode: Text.WordWrap }
        Label { Layout.fillWidth: true; text: card.capability.origin || qsTr("Page locale"); textFormat: Text.PlainText; color: "#D4E4F7"; font.pixelSize: 11; wrapMode: Text.WrapAnywhere }
        Label { Layout.fillWidth: true; text: card.capability.pending ? qsTr("Demande d’accès reçue") : card.capability.key === "network" ? qsTr("Observation du profil") : card.capability.key === "cookies" ? qsTr("Politique du profil") : [qsTr("Toujours demander"), qsTr("Autorisé"), qsTr("Bloqué pour les prochaines demandes"), qsTr("Fonction indisponible"), qsTr("Recharger pour appliquer")][card.capability.policy || 0]; color: "#FFFFFF"; wrapMode: Text.WordWrap; font.bold: true }
        Label { Layout.fillWidth: true; text: card.capability.reason || ""; color: "#C4D6EB"; font.pixelSize: 12; wrapMode: Text.WordWrap }
        Label { visible: !!card.capability.requester; Layout.fillWidth: true; text: qsTr("Origine demandeuse : ") + (card.capability.requester || ""); textFormat: Text.PlainText; color: "#FFD08A"; font.pixelSize: 11; wrapMode: Text.WrapAnywhere }
        RowLayout {
            visible: !!card.capability.pending
            Button { text: qsTr("Autoriser cette demande"); onClicked: card.decisionRequested(true) }
            Button { text: qsTr("Refuser"); onClicked: card.decisionRequested(false) }
        }
        Flow {
            Layout.fillWidth: true; spacing: 5
            visible: !!card.capability.canAct && ["camera", "mic", "location", "notifications", "clipboard"].includes(card.capability.key)
            Button { text: qsTr("Demander"); onClicked: card.permissionRequested(card.capability.key, 0) }
            Button { visible: !!card.capability.persistent; text: qsTr("Toujours autoriser"); onClicked: card.permissionRequested(card.capability.key, 1) }
            Button { text: qsTr("Bloquer"); onClicked: card.permissionRequested(card.capability.key, 2) }
        }
        Flow {
            Layout.fillWidth: true; spacing: 5
            visible: !!card.capability.canAct && ["javascript", "storage", "popups"].includes(card.capability.key)
            Button { text: qsTr("Autoriser…"); onClicked: card.policyRequested(card.capability.key, 1) }
            Button { text: qsTr("Bloquer…"); onClicked: card.policyRequested(card.capability.key, 2) }
            Button { visible: card.capability.key === "popups"; text: qsTr("Gestes explicites"); onClicked: card.policyRequested(card.capability.key, 0) }
        }
        Button { visible: card.capability.policy === 4; text: qsTr("Recharger l’onglet"); onClicked: card.reloadRequested() }
        Button { visible: card.capability.key === "cookies"; text: browserStore.blockThirdPartyCookies ? qsTr("Autoriser les cookies tiers du profil") : qsTr("Bloquer les cookies tiers du profil"); onClicked: card.cookieRequested() }
        Label { visible: !!card.capability.recent && card.capability.key !== "network"; Layout.fillWidth: true; text: qsTr("Événements Qt récents (60 s), pas une preuve d’utilisation active."); color: "#A9C1DC"; font.pixelSize: 11; wrapMode: Text.WordWrap }
        Label { visible: card.capability.key === "network"; text: qsTr("Échantillon du profil : ") + (card.capability.sampleCount || 0); color: "#A9C1DC"; font.pixelSize: 11 }
        Repeater {
            model: card.capability.events || []
            delegate: Label {
                required property var modelData
                Layout.fillWidth: true
                text: new Date(modelData.time).toLocaleTimeString(Qt.locale(), "HH:mm:ss") + "  " + modelData.origin + (modelData.resourceType !== undefined ? " · type " + modelData.resourceType : modelData.blocked ? " · " + qsTr("Refus") : "")
                textFormat: Text.PlainText; color: "#BBD2EA"; font.pixelSize: 10; wrapMode: Text.WrapAnywhere
            }
        }
        }
    }
}
