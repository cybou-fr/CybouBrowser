import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string hint: ""
    implicitWidth: 30
    implicitHeight: 28
    padding: 0
    hoverEnabled: true
    Accessible.name: hint.length ? hint : text
    ToolTip.visible: hovered && hint.length > 0
    ToolTip.text: hint
    contentItem: Text {
        text: control.text
        color: control.enabled ? (control.hovered ? "#FFFFFF" : "#A0B3D0") : "#41536E"
        font.pixelSize: 17
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 7
        gradient: Gradient {
            GradientStop { position: 0.0; color: control.down ? "#CC18355B" : control.hovered ? "#90244673" : "transparent" }
            GradientStop { position: 1.0; color: control.down ? "#DD0E223D" : control.hovered ? "#60142C4E" : "transparent" }
        }
        color: "transparent"
        border.width: control.hovered ? 1 : 0
        border.color: "#407DA5CC"
        Rectangle {
            visible: control.hovered
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
            height: 1; color: "#35FFFFFF"; radius: parent.radius
        }
    }
}
