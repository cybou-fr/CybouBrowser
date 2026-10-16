import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: picker
    width: 208; padding: 12; focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { radius: 14; color: "#F0203550"; border.color: "#70B6CDE5" }
    contentItem: ColumnLayout {
        spacing: 8
        Label { text: qsTr("Position du panneau"); color: "white"; font.bold: true }
        Repeater {
            model: [{key:"top", label:qsTr("Haut")}, {key:"right",label:qsTr("Droite")}, {key:"bottom",label:qsTr("Bas")}, {key:"left",label:qsTr("Gauche")}]
            delegate: Button {
                required property var modelData
                Layout.fillWidth: true
                text: (browserStore.controlBarPosition === modelData.key ? "✓  " : "□  ") + modelData.label
                onClicked: { browserStore.controlBarPosition = modelData.key; picker.close() }
            }
        }
    }
}
