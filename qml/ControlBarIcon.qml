import QtQuick
import QtQuick.Controls

Button {
    id: icon
    required property var capability
    property bool vertical: false
    property bool separator: false
    property bool previousPending: false
    property bool tabStop: false
    signal hoverChanged(bool inside)
    signal focusStep(int direction)
    implicitWidth: 32; implicitHeight: 32
    padding: 7; hoverEnabled: true
    focusPolicy: tabStop ? Qt.StrongFocus : Qt.ClickFocus
    Accessible.name: capability.title + " · " + (capability.key === "position" ? "" : [qsTr("Demander"), qsTr("Autorisé"), qsTr("Bloqué"), qsTr("Indisponible"), qsTr("Rechargement requis")][capability.policy || 0] + " · ") + (capability.pending ? qsTr("Demande en attente") + " · " : "") + capability.scope + " · " + capability.reason + " · " + qsTr("Ouvrir les détails")
    onHoveredChanged: hoverChanged(hovered)
    Keys.onPressed: function(event) {
        if ([Qt.Key_Left, Qt.Key_Up, Qt.Key_Right, Qt.Key_Down].includes(event.key)) {
            focusStep(event.key === Qt.Key_Left || event.key === Qt.Key_Up ? -1 : 1); event.accepted = true
        }
    }
    background: Rectangle {
        radius: 12
        color: icon.down ? "#40FFFFFF" : icon.hovered || icon.activeFocus ? "#24FFFFFF" : "transparent"
        border.width: icon.activeFocus ? 1 : 0; border.color: "#C7E3FF"
    }
    contentItem: Image {
        source: "qrc:/assets/controlbar/" + icon.capability.key + ".svg"
        sourceSize.width: 20; sourceSize.height: 20; fillMode: Image.PreserveAspectFit
        opacity: icon.capability.policy === 3 ? 0.5 : 1
    }
    Rectangle {
        visible: icon.separator
        x: icon.vertical ? 6 : 0; y: icon.vertical ? 0 : 6
        width: icon.vertical ? 22 : 1; height: icon.vertical ? 1 : 22
        color: "#32FFFFFF"
    }
    Rectangle {
        visible: icon.capability.key !== "position"
        x: parent.width - 10; y: 3; width: 9; height: 9; radius: 4.5
        color: icon.capability.pending ? "#FFBE58" : icon.capability.policy === 1 ? "#74E6AF" : icon.capability.policy === 2 ? "#FF7786" : "#1A304A"
        border.color: "#809EBB"; border.width: 0.5
        Text { anchors.centerIn: parent; text: icon.capability.pending ? "•" : icon.capability.policy === 1 ? "✓" : icon.capability.policy === 2 ? "−" : "?"; color: "#F4F8FF"; font.pixelSize: 7 }
    }
    Rectangle {
        visible: !!icon.capability.recent && !icon.capability.pending
        x: parent.width - 7; y: parent.height - 7; width: 4; height: 4; radius: 2
        color: icon.capability.blocked ? "#FF7786" : "#FFD08A"
    }
    SequentialAnimation {
        id: requestPulse
        loops: 2
        NumberAnimation { target: icon; property: "opacity"; to: 0.55; duration: 180 }
        NumberAnimation { target: icon; property: "opacity"; to: 1; duration: 220 }
    }
    onCapabilityChanged: {
        const pending = !!capability.pending
        if (pending && !previousPending && !browserStore.reduceMotion && !browserStore.disableGlow) requestPulse.start()
        previousPending = pending
    }
}
