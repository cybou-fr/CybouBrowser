import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

Rectangle {
    id: tile
    property var tileData: (typeof modelData !== "undefined") ? modelData : null
    signal triggered(string url)

    implicitWidth: 92
    implicitHeight: 96
    radius: 12
    color: tileMouse.pressed ? "#3538BDF8" : tileMouse.containsMouse ? "#2038BDF8" : "#0D1E36"
    border.color: tileMouse.containsMouse ? "#608BB8" : "#204060"
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
            if (tile.tileData && tile.tileData.url) {
                tile.triggered(tile.tileData.url)
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 6

        // Icon badge
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 44
            implicitHeight: 44
            radius: 12
            gradient: Gradient {
                GradientStop { position: 0.0; color: tileMouse.containsMouse ? "#5038BDF8" : "#3038BDF8" }
                GradientStop { position: 1.0; color: tileMouse.containsMouse ? "#301D4ED8" : "#181D4ED8" }
            }
            border.color: tileMouse.containsMouse ? "#8038BDF8" : "#4038BDF8"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: {
                    const r = tile.tileData ? tile.tileData.role : ""
                    if (r === "mail") return "✉"
                    if (r === "calendar") return "📅"
                    if (r === "files") return "📁"
                    if (r === "docs") return "📄"
                    if (r === "sheets") return "📊"
                    if (r === "presentations") return "📽"
                    if (r === "meetings") return "📹"
                    return "☁"
                }
                font.pixelSize: 22
                color: "#FFFFFF"
            }
        }

        // App name
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.maximumWidth: 86
            text: tile.tileData ? tile.tileData.name : ""
            color: tileMouse.containsMouse ? "#FFFFFF" : "#E2E8F0"
            font.pixelSize: 11
            font.bold: true
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }
    }

    GlassToolTip {
        parent: tile
        visible: tileMouse.containsMouse && !tileMouse.pressed
        delay: 500
        text: tile.tileData ? (tile.tileData.roleName + " — " + tile.tileData.name) : ""
    }
}
