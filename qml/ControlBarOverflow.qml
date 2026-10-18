import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Menu {
    id: overflow
    required property var controller
    property var frozenKeys: []
    signal chosen(string key)
    width: 300
    padding: 8
    background: Rectangle {
        radius: 14
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
    function present(keys) { frozenKeys = keys.slice(); open() }
    Repeater {
        model: overflow.frozenKeys
        MenuItem {
            id: entry
            required property string modelData
            property var row: { overflow.controller.revision; return overflow.controller.capability(modelData) }
            text: row.title || modelData
            Accessible.name: text + " · " + iconState.Accessible.name
            contentItem: Item {
                implicitHeight: 28
                Row {
                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; spacing: 8
                    ControlBarIcon { id: iconState; capability: entry.row; focusPolicy: Qt.NoFocus; enabled: false; width: 30; height: 26 }
                    Label { textFormat: Text.PlainText; text: entry.text; color: "#FFFFFF"; font.pixelSize: 13; height: 28; verticalAlignment: Text.AlignVCenter }
                }
                Label {
                    textFormat: Text.PlainText
                    anchors.right: parent.right; anchors.rightMargin: 6; anchors.verticalCenter: parent.verticalCenter
                    readonly property bool activeDeny: entry.row.policy === 2 && (!!entry.row.explicitDeny || !!entry.row.blocked)
                    text: entry.row.pending ? qsTr("Demande") : entry.row.policy === 1 ? qsTr("Autorisé") : activeDeny ? qsTr("Bloqué") : entry.row.policy === 4 ? qsTr("Rechargement") : qsTr("Demander")
                    color: entry.row.pending ? "#F59E0B" : entry.row.policy === 1 ? "#10B981" : activeDeny ? "#EF4444" : "#8FA3BF"
                    font.pixelSize: 12; font.bold: true
                }
            }
            background: Rectangle { color: entry.highlighted ? "#16355E" : "transparent"; radius: 6 }
            onTriggered: { overflow.close(); overflow.chosen(modelData) }
        }
    }
}
