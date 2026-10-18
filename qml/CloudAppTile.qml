import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

Item {
    id: tile
    property var tileData: (typeof modelData !== "undefined") ? modelData : null
    signal triggered(string appId, string url)
    signal configureRequested()

    implicitWidth: 94
    implicitHeight: 82

    focus: true
    activeFocusOnTab: true

    function activateTile() {
        if (!tile.tileData) return
        if (tile.tileData.requiresConfiguration) {
            tile.configureRequested()
        } else {
            tile.triggered(tile.tileData.appId || "", tile.tileData.url || "")
        }
    }

    Keys.onReturnPressed: activateTile()
    Keys.onSpacePressed: activateTile()

    // Smooth hover / active highlight background (transparent at rest)
    Rectangle {
        id: bgHighlight
        anchors.fill: parent
        radius: 12
        color: tileMouse.pressed ? "#2E38BDF8" : (tile.activeFocus || tileMouse.containsMouse) ? "#1838BDF8" : "transparent"
        border.color: tile.activeFocus ? "#38BDF8" : (tileMouse.containsMouse ? "#4038BDF8" : "transparent")
        border.width: tile.activeFocus ? 1.5 : 1

        Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
        Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
    }

    MouseArea {
        id: tileMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.activateTile()
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 5

        // Vector SVG icon (34x34)
        Item {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 34
            implicitHeight: 34

            Image {
                id: appIcon
                anchors.centerIn: parent
                width: 32
                height: 32
                sourceSize.width: 32
                sourceSize.height: 32
                fillMode: Image.PreserveAspectFit
                source: {
                    const iconName = tile.tileData ? (tile.tileData.iconId || tile.tileData.icon || "cloud") : "cloud"
                    return "qrc:/assets/cloud/" + iconName + ".svg"
                }
                smooth: true
                opacity: tileMouse.pressed ? 0.75 : 1.0
            }
        }

        // App display name (12px, clean contrast)
        Text {
            textFormat: Text.PlainText
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.maximumWidth: 88
            text: tile.tileData ? tile.tileData.name : ""
            color: (tile.activeFocus || tileMouse.containsMouse) ? "#FFFFFF" : "#E2E8F0"
            font.pixelSize: 12
            font.bold: true
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }

        // Subtitle or configuration alert badge
        Text {
            textFormat: Text.PlainText
            Layout.alignment: Qt.AlignHCenter
            text: {
                if (!tile.tileData) return ""
                if (tile.tileData.requiresConfiguration) return qsTr("À configurer")
                if (tile.tileData.kind === 1) return qsTr("Portail")
                if (tile.tileData.kind === 2) return qsTr("Info")
                return tile.tileData.providerName || ""
            }
            color: (tile.tileData && tile.tileData.requiresConfiguration) ? "#F59E0B" : "#7A8FA6"
            font.pixelSize: 9
            font.bold: tile.tileData && tile.tileData.requiresConfiguration
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
