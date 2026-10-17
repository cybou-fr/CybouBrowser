import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Cybou

Item {
    id: launcher
    implicitWidth: launcherRow.implicitWidth
    implicitHeight: 36

    signal navigateRequested(string url)
    signal settingsRequested()

    readonly property var cloudMgr: (typeof browserStore !== "undefined" && browserStore.cloud) ? browserStore.cloud : null
    readonly property bool isCloudEnabled: cloudMgr ? cloudMgr.isEnabled : false
    visible: isCloudEnabled

    RowLayout {
        id: launcherRow
        anchors.fill: parent
        spacing: 10

        // "Почта" button
        Button {
            id: mailButton
            implicitHeight: 34
            implicitWidth: mailRow.implicitWidth + 24
            hoverEnabled: true
            scale: down ? 0.95 : hovered ? 1.03 : 1.0
            Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100 } }

            onClicked: {
                if (cloudMgr) {
                    const mail = cloudMgr.mailUrl()
                    if (mail.toString().length > 0) {
                        launcher.navigateRequested(mail.toString())
                    }
                }
            }

            contentItem: RowLayout {
                id: mailRow
                spacing: 6
                Text {
                    text: "✉"
                    color: mailButton.hovered ? "#38BDF8" : "#94A3B8"
                    font.pixelSize: 13
                }
                Text {
                    text: qsTr("Почта")
                    color: mailButton.hovered ? "#FFFFFF" : "#E2E8F0"
                    font.pixelSize: 13
                    font.bold: true
                }
            }

            background: Rectangle {
                radius: 17
                gradient: Gradient {
                    GradientStop { position: 0.0; color: mailButton.down ? "#DD0D2038" : mailButton.hovered ? "#CC1E3D68" : "#80122B4C" }
                    GradientStop { position: 1.0; color: mailButton.down ? "#EE071222" : mailButton.hovered ? "#DD132948" : "#900A1A30" }
                }
                border.color: mailButton.hovered ? "#608BB8" : CybouTheme.borderHighlight
                border.width: 1
                Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
                Rectangle {
                    anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                    height: 1; color: CybouTheme.glassSheenSubtle; radius: parent.radius
                }
            }

            GlassToolTip {
                parent: mailButton
                visible: mailButton.hovered && !mailButton.down
                delay: 600
                text: {
                    if (!cloudMgr) return qsTr("Courrier")
                    const m = cloudMgr.resolveRole("mail")
                    return m.name ? (m.name + " (" + m.roleName + ")") : qsTr("Courrier")
                }
            }
        }

        // 3x3 dots button (Grid App Launcher)
        Button {
            id: gridButton
            implicitWidth: 34
            implicitHeight: 34
            hoverEnabled: true
            scale: down ? 0.95 : hovered ? 1.05 : 1.0
            Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100 } }

            onClicked: {
                if (launcherPopup.visible) {
                    launcherPopup.close()
                } else {
                    launcherPopup.open()
                }
            }

            contentItem: Grid {
                anchors.centerIn: parent
                columns: 3
                rows: 3
                spacing: 3

                Repeater {
                    model: 9
                    Rectangle {
                        width: 3.5
                        height: 3.5
                        radius: 1.75
                        color: gridButton.hovered ? "#38BDF8" : "#CBD5E1"
                    }
                }
            }

            background: Rectangle {
                radius: 17
                gradient: Gradient {
                    GradientStop { position: 0.0; color: gridButton.down ? "#DD0D2038" : gridButton.hovered ? "#CC1E3D68" : "#80122B4C" }
                    GradientStop { position: 1.0; color: gridButton.down ? "#EE071222" : gridButton.hovered ? "#DD132948" : "#900A1A30" }
                }
                border.color: gridButton.hovered ? "#608BB8" : CybouTheme.borderHighlight
                border.width: 1
                Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
                Rectangle {
                    anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                    height: 1; color: CybouTheme.glassSheenSubtle; radius: parent.radius
                }
            }

            GlassToolTip {
                parent: gridButton
                visible: gridButton.hovered && !launcherPopup.visible
                delay: 600
                text: qsTr("Applications cloud (CYBOU Cloud)")
            }
        }
    }

    // Floating Glass Popup for the 3x3 App Launcher
    Popup {
        id: launcherPopup
        parent: Overlay.overlay
        x: Math.min(Overlay.overlay.width - width - 18, Math.max(10, gridButton.mapToItem(Overlay.overlay, 0, 0).x - width + gridButton.width))
        y: gridButton.mapToItem(Overlay.overlay, 0, 0).y + gridButton.height + 8
        width: 300
        padding: 12
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 16
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#E80E2444" }
                GradientStop { position: 1.0; color: "#F4061326" }
            }
            border.color: CybouTheme.borderHighlight
            border.width: 1.5

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#B0000000"
                shadowBlur: 1.0
                shadowVerticalOffset: 12
            }

            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                height: 1; color: CybouTheme.glassSheen; radius: parent.radius
            }
        }

        contentItem: ColumnLayout {
            spacing: 12

            // Header with provider info
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Applications cloud")
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.bold: true
                    Layout.fillWidth: true
                }
                Button {
                    text: "⚙"
                    implicitWidth: 26
                    implicitHeight: 26
                    padding: 0
                    Accessible.name: qsTr("Configurer les services cloud")
                    contentItem: Text {
                        text: "⚙"
                        color: parent.hovered ? "#38BDF8" : "#94A3B8"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#2038BDF8" : "transparent"
                        radius: 6
                    }
                    onClicked: {
                        launcherPopup.close()
                        launcher.settingsRequested()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: "#205080"
            }

            // Grid of cloud app tiles (3 columns)
            GridLayout {
                id: appsGrid
                columns: 3
                rowSpacing: 10
                columnSpacing: 10
                Layout.alignment: Qt.AlignHCenter

                Repeater {
                    model: cloudMgr ? cloudMgr.resolvedServices : []
                    delegate: CloudAppTile {
                        modelData: modelData
                        onTriggered: function(url) {
                            launcherPopup.close()
                            launcher.navigateRequested(url)
                        }
                    }
                }
            }

            Label {
                visible: appsGrid.children.length <= 1
                text: qsTr("Aucune application disponible.")
                color: "#64748B"
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
