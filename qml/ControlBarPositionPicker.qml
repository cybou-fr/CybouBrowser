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
            id: choices
            model: [{key:"top", label:qsTr("Haut")}, {key:"right",label:qsTr("Droite")}, {key:"bottom",label:qsTr("Bas")}, {key:"left",label:qsTr("Gauche")}]
            delegate: Button {
                id: choice
                required property var modelData
                Layout.fillWidth: true
                Accessible.name: modelData.label
                contentItem: RowLayout {
                    spacing: 12
                    Rectangle {
                        implicitWidth: 28; implicitHeight: 20; radius: 3; color: "transparent"; border.color: "#65758E"
                        Rectangle {
                            readonly property bool vertical: ["left", "right"].includes(choice.modelData.key)
                            x: choice.modelData.key === "right" ? 22 : 3
                            y: choice.modelData.key === "bottom" ? 14 : 3
                            width: vertical ? 3 : 22; height: vertical ? 14 : 3
                            color: "#0D2547"; radius: 1
                        }
                    }
                    Label { text: choice.modelData.label; color: "#0D2547"; Layout.fillWidth: true }
                    Label { text: browserStore.controlBarPosition === choice.modelData.key ? "✓" : ""; color: "#0D2547" }
                }
                onClicked: { browserStore.controlBarPosition = modelData.key; picker.close() }
            }
        }
    }
    function testChoicePoint(index) { const item = choices.itemAt(index); const point = item.mapToItem(null, item.width / 2, item.height / 2); return {kind:"click",x:point.x,y:point.y} }
}
