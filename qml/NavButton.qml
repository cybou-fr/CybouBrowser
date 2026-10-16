import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string hint: ""
    implicitWidth: 38
    implicitHeight: 36
    hoverEnabled: true
    Accessible.name: hint.length ? hint : text
    ToolTip.visible: hovered && hint.length > 0
    ToolTip.text: hint
    contentItem: Text {
        text: control.text
        color: control.enabled ? "#0D2547" : "#AAB3C1"
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 10
        color: control.down ? "#DCE3ED" : control.hovered ? "#E8EDF4" : "transparent"
    }
}
