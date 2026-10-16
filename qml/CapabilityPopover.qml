import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Cybou

Popup {
    id: card
    property var capability: ({})
    property bool pinned: false
    property bool pointerInside: hover.hovered
    property real minimumY: 0
    signal policyRequested(string key, int mode)
    signal permissionRequested(string key, int mode)
    signal decisionRequested(bool allow)
    signal reloadRequested()
    signal cookieRequested()

    component GlassBtn: Button {
        id: gbtn
        scale: down ? 0.95 : hovered ? 1.02 : 1.0
        Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100 } }
        contentItem: Text {
            text: gbtn.text; color: gbtn.highlighted ? "#FFFFFF" : gbtn.enabled ? CybouTheme.mainText : CybouTheme.mutedText
            font.pixelSize: 12; font.bold: gbtn.highlighted || gbtn.hovered
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 6
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
    width: 330; padding: 16
    height: Math.min(implicitHeight, parent.height - minimumY - 8)
    focus: pinned; modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: 16
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#F00E223D" }
            GradientStop { position: 1.0; color: "#F8061120" }
        }
        border.color: "#497DA5CC"
        border.width: 1.5
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#80000000"
            shadowBlur: 1.0
            shadowVerticalOffset: 12
        }
        Rectangle {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
            height: 1; color: "#50FFFFFF"; radius: parent.radius
        }
    }
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
            spacing: 10
            Rectangle {
                width: 36; height: 36; radius: 8
                color: "#122540"; border.color: "#304D75"; border.width: 1
                Image {
                    id: popGlyph
                    anchors.centerIn: parent; width: 20; height: 20
                    source: card.capability.key ? "qrc:/assets/controlbar/" + card.capability.key + ".svg" : ""
                    sourceSize.width: 36; sourceSize.height: 36
                    fillMode: Image.PreserveAspectFit; visible: false
                }
                MultiEffect {
                    anchors.fill: popGlyph; source: popGlyph; colorization: 1
                    colorizationColor: {
                        switch (card.capability.key) {
                        case "camera": return "#FF4D4D"
                        case "mic": return "#10B981"
                        case "location": return "#38BDF8"
                        case "notifications": return "#F59E0B"
                        case "javascript": return "#10B981"
                        case "popups": return "#FB923C"
                        case "cookies": return "#C084FC"
                        case "storage": return "#818CF8"
                        case "clipboard": return "#38BDF8"
                        case "network": return "#2DD4BF"
                        default: return "#94A3B8"
                        }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 1
                Label { text: card.capability.title || ""; color: "white"; font.bold: true; font.pixelSize: 15 }
                Label { text: card.capability.origin || qsTr("Page locale"); textFormat: Text.PlainText; color: "#7DD3FC"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
            }
            Button {
                text: "×"; implicitWidth: 26; implicitHeight: 26; onClicked: card.close(); Accessible.name: qsTr("Fermer les détails")
                contentItem: Text { text: "×"; color: parent.hovered ? "#FFFFFF" : "#94A3B8"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { radius: 6; color: parent.hovered ? "#E11D2E" : "#142948" }
            }
        }
        Label { Layout.fillWidth: true; text: card.capability.scope || ""; color: "#94A3B8"; font.pixelSize: 11; wrapMode: Text.WordWrap }
        Label { Layout.fillWidth: true; text: card.capability.pending ? qsTr("Demande d’accès reçue") : card.capability.key === "network" ? qsTr("Observation du profil") : card.capability.key === "cookies" ? qsTr("Politique du profil") : [qsTr("Accès bloqué sans demande"), qsTr("Autorisé"), qsTr("Bloqué pour les prochaines demandes"), qsTr("Fonction indisponible"), qsTr("Recharger pour appliquer")][card.capability.policy || 0]; color: "#FFFFFF"; wrapMode: Text.WordWrap; font.bold: true }
        Label { Layout.fillWidth: true; text: card.capability.reason || ""; color: "#CBD5E1"; font.pixelSize: 12; wrapMode: Text.WordWrap }
        Label { visible: !!card.capability.requester; Layout.fillWidth: true; text: qsTr("Origine demandeuse : ") + (card.capability.requester || ""); textFormat: Text.PlainText; color: "#FFD08A"; font.pixelSize: 11; wrapMode: Text.WrapAnywhere }
        RowLayout {
            visible: !!card.capability.pending
            GlassBtn { text: qsTr("Autoriser cette demande"); onClicked: card.decisionRequested(true) }
            GlassBtn { text: qsTr("Refuser"); onClicked: card.decisionRequested(false) }
        }
        Flow {
            Layout.fillWidth: true; spacing: 5
            visible: !!card.capability.canAct && ["camera", "mic", "location", "notifications", "clipboard"].includes(card.capability.key)
            GlassBtn { text: qsTr("Réinitialiser le refus"); onClicked: card.permissionRequested(card.capability.key, 0) }
            GlassBtn { text: qsTr("Bloquer"); onClicked: card.permissionRequested(card.capability.key, 2) }
        }
        Flow {
            Layout.fillWidth: true; spacing: 5
            visible: !!card.capability.canAct && ["javascript", "storage", "popups"].includes(card.capability.key)
            GlassBtn { text: qsTr("Autoriser"); onClicked: card.policyRequested(card.capability.key, 1) }
            GlassBtn { text: qsTr("Bloquer"); onClicked: card.policyRequested(card.capability.key, 2) }
            GlassBtn { visible: card.capability.key === "popups"; text: qsTr("Gestes explicites"); onClicked: card.policyRequested(card.capability.key, 0) }
        }
        Label { visible: ["camera", "mic"].includes(card.capability.key); Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#C4D6EB"; font.pixelSize: 11; text: qsTr("Pour interrompre un flux déjà accordé, rechargez ou fermez l’onglet. L’arrêt n’est pas vérifiable ici.") }
        GlassBtn { visible: card.capability.policy === 4 || (!!card.capability.canAct && ["camera", "mic"].includes(card.capability.key)); text: qsTr("Recharger l’onglet"); onClicked: card.reloadRequested() }
        GlassBtn { visible: card.capability.key === "cookies"; text: browserStore.blockThirdPartyCookies ? qsTr("Autoriser les cookies tiers du profil") : qsTr("Bloquer les cookies tiers du profil"); onClicked: card.cookieRequested() }
        ColumnLayout {
            visible: card.capability.key === "network"
            Layout.fillWidth: true; spacing: 4
            Label {
                text: qsTr("Requêtes filtrées · tous les profils : %1").arg(browserStore.totalBlockedTrackers())
                color: "#38BDF8"; font.pixelSize: 12; font.bold: true
            }
            Label {
                text: qsTr("Échantillon réseau récent : %1").arg(card.capability.sampleCount || 0)
                color: "#A9C1DC"; font.pixelSize: 11
            }
            GlassBtn {
                text: browserStore.blockTrackers ? qsTr("Désactiver le bloqueur") : qsTr("Activer le bloqueur de traqueurs")
                enabled: false
            }
        }
        Label { visible: !!card.capability.recent && card.capability.key !== "network"; Layout.fillWidth: true; text: qsTr("Événements Qt récents (60 s), pas une preuve d’utilisation active."); color: "#A9C1DC"; font.pixelSize: 11; wrapMode: Text.WordWrap }
        Repeater {
            model: card.capability.events || []
            delegate: Label {
                required property var modelData
                Layout.fillWidth: true
                text: new Date(modelData.time).toLocaleTimeString(Qt.locale(), "HH:mm:ss") + "  " + modelData.origin + (modelData.resourceType !== undefined ? " · type " + modelData.resourceType : "") + (modelData.blocked ? " · " + qsTr("Bloqué") + " · " + (modelData.reason || "") : "")
                textFormat: Text.PlainText; color: "#BBD2EA"; font.pixelSize: 10; wrapMode: Text.WrapAnywhere
            }
        }
        }
    }
}
