import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

Rectangle {
    id: tile
    required property var modelData
    signal triggered(string url)

    implicitWidth: 84
    implicitHeight: 88
    radius: 12
    color: tileMouse.containsMouse ? "#2A3B5C88" : "#10254060"
    border.color: tileMouse.containsMouse ? "#608BB8" : "transparent"
    border.width: 1

    Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
    Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }

    scale: tileMouse.pressed ? 0.94 : tileMouse.containsMouse ? 1.04 : 1.0
    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }

    MouseArea {
        id: tileMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (tile.modelData && tile.modelData.url) {
                tile.triggered(tile.modelData.url)
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 6

        // Icon badge
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 42
            implicitHeight: 42
            radius: 10
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#4038BDF8" }
                GradientStop { position: 1.0; color: "#201D4ED8" }
            }
            border.color: "#3038BDF8"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: {
                    const r = tile.modelData ? tile.modelData.role : ""
                    if (r === "mail") return "✉"
                    if (r === "calendar") return "📅"
                    if (r === "files") return "📁"
                    if (r === "docs") return "📄"
                    if (r === "sheets") return "📊"
                    if (r === "presentations") return "📽"
                    if (r === "meetings") return "📹"
                    return "☁"
                }
                font.pixelSize: 20
                color: "#FFFFFF"
            }
        }

        // App name
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 76
            text: tile.modelData ? tile.modelData.name : ""
            color: "#E2E8F0"
            font.pixelSize: 11
            font.bold: true
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }
    }

    GlassToolTip {
        parent: tile
        visible: tileMouse.containsMouse && !tileMouse.pressed
        delay: 500
        text: tile.modelData ? (tile.modelData.roleName + " — " + tile.modelData.name) : ""
    }
}
