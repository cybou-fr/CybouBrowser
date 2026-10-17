import QtQuick
import QtQuick.Controls
import Cybou

MenuItem {
    id: gitem
    implicitHeight: 32
    scale: down ? 0.98 : 1.0
    contentItem: Text {
        leftPadding: 8; rightPadding: 8
        text: gitem.text
        color: gitem.enabled ? (gitem.highlighted ? "#FFFFFF" : CybouTheme.mainText) : CybouTheme.mutedText
        font.pixelSize: 13
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 6
        gradient: Gradient {
            GradientStop { position: 0.0; color: gitem.highlighted ? "#50285285" : "transparent" }
            GradientStop { position: 1.0; color: gitem.highlighted ? "#40183864" : "transparent" }
        }
        border.color: gitem.highlighted ? CybouTheme.borderHighlight : "transparent"
        border.width: 1
        Rectangle {
            visible: gitem.highlighted
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
            height: 1; color: "#35FFFFFF"; radius: parent.radius
        }
    }
}
