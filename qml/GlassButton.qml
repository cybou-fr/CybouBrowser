import QtQuick
import QtQuick.Controls

Button {
    id: gbtn
    scale: down ? 0.95 : hovered ? 1.03 : 1.0
    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100 } }
    contentItem: Text {
        text: gbtn.text; color: gbtn.highlighted ? "#FFFFFF" : gbtn.enabled ? CybouTheme.mainText : CybouTheme.mutedText
        font.pixelSize: 13; font.bold: gbtn.highlighted || gbtn.hovered
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 8
        gradient: Gradient {
            GradientStop { position: 0.0; color: gbtn.highlighted ? "#E0183A68" : gbtn.down ? "#DD0D2038" : gbtn.hovered ? "#DD224572" : "#80142F52" }
            GradientStop { position: 1.0; color: gbtn.highlighted ? "#E80E2444" : gbtn.down ? "#EE071222" : gbtn.hovered ? "#EE163052" : "#900B1D35" }
        }
        border.color: gbtn.highlighted ? CybouTheme.borderActive : gbtn.hovered ? "#7DA5CC" : CybouTheme.borderHighlight
        border.width: 1
        Rectangle {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
            height: 1; color: gbtn.highlighted ? "#50FFFFFF" : CybouTheme.glassSheenSubtle; radius: parent.radius
        }
    }
}
