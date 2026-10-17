import QtQuick
import QtQuick.Effects
import Cybou

Rectangle {
    id: glassSurface
    property Item sourceItem: null
    property real blurFactor: 1.0
    property color tintTopColor: "#E80E2444"
    property color tintBottomColor: "#F4061326"
    property color borderColor: CybouTheme.borderHighlight
    property color sheenColor: CybouTheme.glassSheen
    property real borderWidth: 1.5

    implicitWidth: 360
    implicitHeight: 400
    radius: 16

    gradient: Gradient {
        GradientStop { position: 0.0; color: glassSurface.tintTopColor }
        GradientStop { position: 1.0; color: glassSurface.tintBottomColor }
    }
    border.color: glassSurface.borderColor
    border.width: glassSurface.borderWidth

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#80000000"
        shadowBlur: 0.6
        shadowVerticalOffset: 8
    }

    // Top specular sheen line
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        height: 1
        color: glassSurface.sheenColor
        radius: parent.radius
    }
}
