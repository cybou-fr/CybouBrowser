import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

Rectangle {
    id: tile
    property var tileData: (typeof modelData !== "undefined") ? modelData : null
    signal triggered(string url)
    signal configureRequested()

    implicitWidth: 92
    implicitHeight: 98
    radius: 12
    color: tileMouse.pressed ? "#3538BDF8" : (tile.activeFocus || tileMouse.containsMouse) ? "#2038BDF8" : "#0D1E36"
    border.color: tile.activeFocus ? "#38BDF8" : tileMouse.containsMouse ? "#608BB8" : "#204060"
    border.width: tile.activeFocus ? 1.5 : 1

    focus: true
    activeFocusOnTab: true

    Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
    Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }

    scale: tileMouse.pressed ? 0.94 : (tile.activeFocus || tileMouse.containsMouse) ? 1.04 : 1.0
    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }

    function activateTile() {
        if (!tile.tileData) return
        if (tile.tileData.requiresConfiguration) {
            tile.configureRequested()
        } else if (tile.tileData.url) {
            tile.triggered(tile.tileData.url)
        }
    }

    Keys.onReturnPressed: activateTile()
    Keys.onSpacePressed: activateTile()

    MouseArea {
        id: tileMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.activateTile()
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 4

        // Icon badge
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 42
            implicitHeight: 42
            radius: 12
            gradient: Gradient {
                GradientStop { position: 0.0; color: (tile.activeFocus || tileMouse.containsMouse) ? "#5038BDF8" : "#3038BDF8" }
                GradientStop { position: 1.0; color: (tile.activeFocus || tileMouse.containsMouse) ? "#301D4ED8" : "#181D4ED8" }
            }
            border.color: (tile.activeFocus || tileMouse.containsMouse) ? "#8038BDF8" : "#4038BDF8"
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
                font.pixelSize: 21
                color: "#FFFFFF"
            }
        }

        // App name
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.maximumWidth: 86
            text: tile.tileData ? tile.tileData.name : ""
            color: (tile.activeFocus || tileMouse.containsMouse) ? "#FFFFFF" : "#E2E8F0"
            font.pixelSize: 11
            font.bold: true
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }

        // Provider label / config hint
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: {
                if (!tile.tileData) return ""
                if (tile.tileData.requiresConfiguration) return qsTr("À configurer")
                return tile.tileData.providerName || ""
            }
            color: (tile.tileData && tile.tileData.requiresConfiguration) ? "#F59E0B" : "#64748B"
            font.pixelSize: 9
            font.bold: tile.tileData && tile.tileData.requiresConfiguration
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }
    }

    GlassToolTip {
        parent: tile
        visible: (tile.activeFocus || tileMouse.containsMouse) && !tileMouse.pressed
        delay: 500
        text: {
            if (!tile.tileData) return ""
            if (tile.tileData.requiresConfiguration) return qsTr("Cliquez pour configurer l’adresse de votre serveur dans les paramètres.")
            return tile.tileData.roleName + " — " + tile.tileData.name + " (" + (tile.tileData.providerName || "") + ")"
        }
    }
}
