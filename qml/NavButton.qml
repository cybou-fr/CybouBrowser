import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string hint: ""
    property bool hintEnabled: true
    readonly property bool hintVisible: glassHint.visible
    implicitWidth: 30
    implicitHeight: 28
    padding: 0
    hoverEnabled: true
    Accessible.name: hint.length ? hint : text
    GlassToolTip { id: glassHint; parent: control; visible: control.hovered && control.enabled && control.hintEnabled && control.hint.length > 0; text: control.hint }
    scale: control.down ? 0.93 : control.hovered ? 1.05 : 1.0
    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }
    // An icon (icon.source) replaces the text glyph; the hint then names the action, as a tooltip and for accessibility.
    contentItem: Item {
        Image {
            visible: control.icon.source.toString().length > 0
            anchors.centerIn: parent
            source: control.icon.source
            sourceSize: Qt.size(18, 18)
            opacity: control.enabled ? (control.hovered || control.highlighted ? 1.0 : 0.72) : 0.3
        }
        Text {
            visible: control.icon.source.toString().length === 0
            anchors.fill: parent
            textFormat: Text.PlainText
            text: control.text
            color: control.enabled ? (control.hovered ? "#FFFFFF" : "#A0B3D0") : "#41536E"
            Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
            font.pixelSize: 17
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    background: Rectangle {
        radius: 7
        gradient: Gradient {
            GradientStop { position: 0.0; color: control.down ? "#CC18355B" : control.hovered ? "#90244673" : control.highlighted ? "#70244673" : "transparent" }
            GradientStop { position: 1.0; color: control.down ? "#DD0E223D" : control.hovered ? "#60142C4E" : control.highlighted ? "#50142C4E" : "transparent" }
        }
        color: "transparent"
        border.width: control.hovered || control.highlighted ? 1 : 0
        border.color: "#407DA5CC"
        Behavior on border.width { NumberAnimation { duration: 100 } }
        Rectangle {
            opacity: control.hovered ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
            height: 1; color: "#35FFFFFF"; radius: parent.radius
        }
    }
}
