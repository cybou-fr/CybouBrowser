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
        color: control.down ? "#1D3B66" : control.hovered ? "#142948" : "transparent"
        border.width: control.hovered ? 1 : 0
        border.color: "#1E4273"
    }
}
