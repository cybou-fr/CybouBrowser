import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

Item {
    id: launcher
    implicitWidth: launcherRow.implicitWidth
    implicitHeight: 36

    property Item wallpaperItem: null
    property real blurFactor: 0.85

    signal navigateRequested(string url)
    signal settingsRequested()

    function openPopup() { launcherPopup.open() }
    function closePopup() { launcherPopup.close() }
    readonly property bool popupVisible: launcherPopup.visible

    readonly property var cloudMgr: (typeof browserStore !== "undefined" && browserStore.cloud) ? browserStore.cloud : null
    readonly property bool isCloudEnabled: cloudMgr ? cloudMgr.isEnabled : false
    visible: isCloudEnabled
    onVisibleChanged: if (!visible) closePopup()

    RowLayout {
        id: launcherRow
        anchors.fill: parent
        spacing: 10

        // "Courrier" (Mail) button - visible ONLY when mail role is available and configured
        Button {
            id: mailButton
            visible: cloudMgr ? cloudMgr.isMailAvailable() : false
            implicitHeight: 34
            implicitWidth: mailRow.implicitWidth + 24
            padding: 0
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
                anchors.centerIn: parent
                spacing: 7
                Image {
                    source: "qrc:/assets/cloud/mail.svg"
                    width: 16
                    height: 16
                    sourceSize.width: 16
                    sourceSize.height: 16
                    fillMode: Image.PreserveAspectFit
                    opacity: mailButton.hovered ? 1.0 : 0.8
                }
                Text {
                    text: qsTr("Courrier")
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
                    return m.name ? (m.name + " (" + (m.providerName || "") + ")") : qsTr("Courrier")
                }
            }
        }

        // 3x3 dots button (Grid App Launcher)
        Button {
            id: gridButton
            implicitWidth: 34
            implicitHeight: 34
            padding: 0
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

            contentItem: Item {
                implicitWidth: 18
                implicitHeight: 18
                Grid {
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

    // Floating Frosted Glass Popup for the App Launcher
    Popup {
        id: launcherPopup
        parent: gridButton
        x: gridButton.width - width
        y: gridButton.height + 8
        width: 336
        implicitHeight: Math.min(500, popupCol.implicitHeight + 28)
        padding: 14
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        background: FrostedGlassSurface {
            sourceItem: launcher.wallpaperItem
            blurFactor: launcher.blurFactor
            radius: 16
        }

        contentItem: ColumnLayout {
            id: popupCol
            spacing: 12

            // Header with title and settings cog
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
                    id: cfgBtn
                    implicitWidth: 26
                    implicitHeight: 26
                    padding: 0
                    hoverEnabled: true
                    Accessible.name: qsTr("Configurer les services cloud")
                    contentItem: Text {
                        text: "⚙"
                        color: cfgBtn.hovered ? "#38BDF8" : "#94A3B8"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: cfgBtn.hovered ? "#2038BDF8" : "transparent"
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

            // Scrollable grid area
            ScrollView {
                Layout.fillWidth: true
                Layout.maximumHeight: 400
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: parent.width
                    spacing: 12

                    // Primary apps section
                    GridLayout {
                        id: appsGrid
                        columns: 3
                        rowSpacing: 10
                        columnSpacing: 8
                        Layout.alignment: Qt.AlignHCenter
                        visible: cloudMgr && cloudMgr.resolvedApps.length > 0

                        Repeater {
                            model: {
                                if (!cloudMgr) return []
                                return cloudMgr.resolvedApps.filter(function(item) {
                                    return item.section !== "other"
                                })
                            }
                            delegate: CloudAppTile {
                                tileData: modelData
                                onTriggered: function(appId, url) {
                                    launcherPopup.close()
                                    let targetUrl = url
                                    if (cloudMgr && appId.length > 0) {
                                        const resolved = cloudMgr.resolveAppUrl(appId)
                                        if (resolved.toString().length > 0) {
                                            targetUrl = resolved.toString()
                                        }
                                    }
                                    if (targetUrl && targetUrl.length > 0) {
                                        launcher.navigateRequested(targetUrl)
                                    }
                                }
                                onConfigureRequested: {
                                    launcherPopup.close()
                                    launcher.settingsRequested()
                                }
                            }
                        }
                    }

                    // Secondary section ("Autres services") for portals or info items
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: otherRepeater.count > 0

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 1
                            color: "#183858"
                        }

                        Label {
                            text: qsTr("Autres services")
                            color: "#94A3B8"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.leftMargin: 4
                        }

                        GridLayout {
                            columns: 3
                            rowSpacing: 10
                            columnSpacing: 8
                            Layout.alignment: Qt.AlignHCenter

                            Repeater {
                                id: otherRepeater
                                model: {
                                    if (!cloudMgr) return []
                                    return cloudMgr.resolvedApps.filter(function(item) {
                                        return item.section === "other"
                                    })
                                }
                                delegate: CloudAppTile {
                                    tileData: modelData
                                    onTriggered: function(appId, url) {
                                        launcherPopup.close()
                                        let targetUrl = url
                                        if (cloudMgr && appId.length > 0) {
                                            const resolved = cloudMgr.resolveAppUrl(appId)
                                            if (resolved.toString().length > 0) {
                                                targetUrl = resolved.toString()
                                            }
                                        }
                                        if (targetUrl && targetUrl.length > 0) {
                                            launcher.navigateRequested(targetUrl)
                                        }
                                    }
                                    onConfigureRequested: {
                                        launcherPopup.close()
                                        launcher.settingsRequested()
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        visible: !cloudMgr || cloudMgr.resolvedApps.length === 0
                        text: qsTr("Aucune application disponible.")
                        color: "#64748B"
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 12
                    }
                }
            }
        }
    }
}
