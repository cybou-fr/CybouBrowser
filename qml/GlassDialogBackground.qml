import QtQuick
import QtQuick.Effects

Rectangle {
    id: root
    radius: CybouTheme.dialogRadius
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#E80E2648" }
        GradientStop { position: 1.0; color: "#F4061224" }
    }
    border.color: CybouTheme.borderHighlight
    border.width: 1.5
    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#A0000000"
        shadowBlur: 1.0
        shadowVerticalOffset: 12
    }
    Rectangle {
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
        height: 1; color: CybouTheme.glassSheen; radius: parent.radius
    }
}
