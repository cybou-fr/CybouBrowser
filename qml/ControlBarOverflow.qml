import QtQuick
import QtQuick.Controls

Menu {
    id: overflow
    required property var controller
    property var frozenKeys: []
    signal chosen(string key)
    width: 290
    padding: 6
    background: Rectangle { color: "#F5F7FB"; radius: 10; border.color: "#CBD7E6" }
    function present(keys) { frozenKeys = keys.slice(); open() }
    Repeater {
        model: overflow.frozenKeys
        MenuItem {
            id: entry
            required property string modelData
            property var row: { overflow.controller.revision; return overflow.controller.capability(modelData) }
            text: row.title || modelData
            Accessible.name: text + " · " + iconState.Accessible.name
            contentItem: Row {
                spacing: 8
                ControlBarIcon { id: iconState; capability: entry.row; focusPolicy: Qt.NoFocus; enabled: false; width: 30; height: 28 }
                Label { text: entry.text; color: "#0D2547"; height: 28; verticalAlignment: Text.AlignVCenter }
            }
            background: Rectangle { color: entry.highlighted ? "#DCE7F5" : "transparent"; radius: 6 }
            onTriggered: { overflow.close(); overflow.chosen(modelData) }
        }
    }
}
