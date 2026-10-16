import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Button {
    id: icon
    required property var capability
    property bool tabStop: false
    readonly property int policy: capability.policy === undefined ? 3 : capability.policy
    readonly property color brandColor: {
        switch (capability.key) {
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
    readonly property color stateColor: capability.pending ? "#F59E0B" : policy === 1 ? brandColor : policy === 2 ? "#EF4444" : policy === 3 ? "#64748B" : policy === 4 ? "#F59E0B" : brandColor
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
        radius: 7
        color: icon.down ? "#1D3B66" : icon.hovered ? "#162E4F" : (icon.policy === 1 || !!icon.capability.pending ? Qt.rgba(icon.stateColor.r, icon.stateColor.g, icon.stateColor.b, 0.12) : (icon.policy === 2 ? "#20EF4444" : "transparent"))
        border.width: icon.activeFocus || (icon.policy === 1 || icon.policy === 2 || !!icon.capability.pending) ? 1 : (icon.hovered ? 1 : 0)
        border.color: icon.activeFocus ? "#38BDF8" : (icon.capability.pending ? "#F59E0B80" : (icon.policy === 1 ? Qt.rgba(icon.brandColor.r, icon.brandColor.g, icon.brandColor.b, 0.35) : (icon.policy === 2 ? "#EF444460" : (icon.hovered ? "#1E3E6B" : "transparent"))))
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
        color: "#081426"; border.color: icon.stateColor; border.width: 1
        Text { anchors.centerIn: parent; text: icon.capability.pending ? "!" : icon.policy === 1 ? "✓" : icon.policy === 2 || icon.policy === 3 ? "−" : "↻"; color: icon.stateColor; font.pixelSize: 8; font.bold: true }
    }
    Rectangle {
        visible: !!icon.capability.recent && !icon.capability.pending && icon.capability.key !== "network"
        x: parent.width - 9; y: parent.height - 6; width: 5; height: 5; radius: 2.5
        color: icon.capability.blocked ? "#EF4444" : "#F59E0B"
    }
}
