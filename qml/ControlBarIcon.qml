import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Button {
    id: icon
    required property var capability
    property bool tabStop: false
    readonly property int policy: capability.policy === undefined ? 3 : capability.policy
    readonly property color stateColor: capability.pending ? "#9C5700" : policy === 1 ? "#176541" : policy === 2 ? "#BC243A" : policy === 3 ? "#778395" : policy === 4 ? "#945A06" : "#26486E"
    readonly property string stateText: capability.pending ? qsTr("Demande en attente") : [qsTr("Demander"), qsTr("Autorisé"), qsTr("Bloqué"), qsTr("Indisponible"), qsTr("Rechargement requis")][policy]
    signal hoverChanged(bool inside)
    signal focusStep(int direction)
    implicitWidth: 40; implicitHeight: 28
    padding: 5; hoverEnabled: true
    focusPolicy: tabStop || activeFocus ? Qt.StrongFocus : Qt.ClickFocus
    Accessible.name: (capability.title || "") + " · " + stateText + " · " + (capability.scope || "") + " · " + (capability.reason || "") + " · " + qsTr("Ouvrir les détails")
    onHoveredChanged: hoverChanged(hovered)
    Keys.onPressed: function(event) {
        if ([Qt.Key_Left, Qt.Key_Up, Qt.Key_Right, Qt.Key_Down].includes(event.key)) {
            focusStep(event.key === Qt.Key_Left || event.key === Qt.Key_Up ? -1 : 1); event.accepted = true
        }
    }
    background: Rectangle {
        radius: 6
        color: icon.down ? "#CCDCEF" : icon.hovered || icon.activeFocus ? "#E4EDF8" : "transparent"
        border.width: icon.activeFocus || browserStore.highContrast ? 1 : 0
        border.color: icon.activeFocus ? "#164A88" : "#8EA1B8"
    }
    contentItem: Item {
        Image {
            id: glyph
            anchors.centerIn: parent; width: 18; height: 18
            source: "qrc:/assets/controlbar/" + icon.capability.key + ".svg"
            sourceSize.width: 36; sourceSize.height: 36
            fillMode: Image.PreserveAspectFit; visible: false
        }
        MultiEffect { anchors.fill: glyph; source: glyph; colorization: 1; colorizationColor: icon.stateColor }
    }
    Rectangle {
        visible: icon.policy !== 0 || !!icon.capability.pending
        x: parent.width - 12; y: 1; width: 10; height: 10; radius: icon.policy === 2 ? 2 : 5
        color: "#F5F7FB"; border.color: icon.stateColor; border.width: 1
        Text { anchors.centerIn: parent; text: icon.capability.pending ? "!" : icon.policy === 1 ? "✓" : icon.policy === 2 || icon.policy === 3 ? "−" : "↻"; color: icon.stateColor; font.pixelSize: 8; font.bold: true }
    }
    Rectangle {
        visible: !!icon.capability.recent && !icon.capability.pending && icon.capability.key !== "network"
        x: parent.width - 9; y: parent.height - 6; width: 4; height: 4; radius: 2
        color: icon.capability.blocked ? "#BC243A" : "#9C5700"
    }
}
