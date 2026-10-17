import QtQuick
import QtQuick.Effects
import Cybou

Item {
    id: glassSurface
    property Item sourceItem: null
    property real blurFactor: 1.0
    property real radius: 18
    property color tintTopColor: "#C80D2138"
    property color tintBottomColor: "#DD071426"
    property color borderColor: CybouTheme.borderHighlight
    property color sheenColor: CybouTheme.glassSheen
    property real borderWidth: 1.5

    implicitWidth: 360
    implicitHeight: 400

    // Ambient drop shadow
    Rectangle {
        anchors.fill: parent
        radius: glassSurface.radius
        color: "transparent"
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#90000000"
            shadowBlur: 1.0
            shadowVerticalOffset: 12
        }
    }

    // Blurred wallpaper slice (aligned to sourceItem coordinates)
    Item {
        id: blurContainer
        anchors.fill: parent
        clip: true
        visible: glassSurface.sourceItem !== null
                 && glassSurface.blurFactor > 0.01
                 && !(typeof browserStore !== "undefined" && browserStore.reduceMotion)

        layer.enabled: visible
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: glassSurface.blurFactor
            blurMax: 32
        }

        ShaderEffectSource {
            id: shaderSource
            property point mapped: (glassSurface.sourceItem && blurContainer.visible)
                                   ? glassSurface.mapToItem(glassSurface.sourceItem, 0, 0)
                                   : Qt.point(0, 0)
            x: -mapped.x
            y: -mapped.y
            width: glassSurface.sourceItem ? glassSurface.sourceItem.width : 0
            height: glassSurface.sourceItem ? glassSurface.sourceItem.height : 0
            sourceItem: glassSurface.sourceItem
            live: true
        }
    }

    // Tinted translucent dark-blue glass wash
    Rectangle {
        anchors.fill: parent
        radius: glassSurface.radius
        gradient: Gradient {
            GradientStop { position: 0.0; color: glassSurface.tintTopColor }
            GradientStop { position: 1.0; color: glassSurface.tintBottomColor }
        }
    }

    // Top specular sheen line
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        height: 1
        color: glassSurface.sheenColor
        radius: glassSurface.radius
    }

    // Crisp glass border
    Rectangle {
        anchors.fill: parent
        radius: glassSurface.radius
        color: "transparent"
        border.color: glassSurface.borderColor
        border.width: glassSurface.borderWidth
    }
}
