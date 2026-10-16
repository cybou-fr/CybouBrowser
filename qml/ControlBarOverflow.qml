import QtQuick
import QtQuick.Controls

Menu {
    id: overflow
    required property var controller
    property var frozenKeys: []
    signal chosen(string key)
    width: 300
    padding: 8
    background: Rectangle { color: "#F2081528"; radius: 14; border.color: "#204675"; border.width: 1.5 }
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
                    Label { text: entry.text; color: "#FFFFFF"; font.pixelSize: 13; height: 28; verticalAlignment: Text.AlignVCenter }
                }
                Label {
                    anchors.right: parent.right; anchors.rightMargin: 6; anchors.verticalCenter: parent.verticalCenter
                    text: entry.row.pending ? qsTr("Demande") : entry.row.policy === 1 ? qsTr("Autorisé") : entry.row.policy === 2 ? qsTr("Bloqué") : entry.row.policy === 4 ? qsTr("Rechargement") : qsTr("Demander")
                    color: entry.row.pending ? "#F59E0B" : entry.row.policy === 1 ? "#10B981" : entry.row.policy === 2 ? "#EF4444" : "#F59E0B"
                    font.pixelSize: 12; font.bold: true
                }
            }
            background: Rectangle { color: entry.highlighted ? "#16355E" : "transparent"; radius: 6 }
            onTriggered: { overflow.close(); overflow.chosen(modelData) }
        }
    }
}
