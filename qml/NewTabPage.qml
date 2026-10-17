import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Cybou

Item {
    id: root
    property bool privateMode: false
    signal navigateRequested(string text)
    signal libraryRequested(string section)
    signal settingsRequested()
    onVisibleChanged: if (!visible) cloudLauncher.closePopup()

    // Cloud Services Launcher (Mail button + 3x3 App Launcher) in top-right corner
    CloudAppLauncher {
        id: cloudLauncher
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        anchors.topMargin: 16
        z: 10
        wallpaperItem: bg
        blurFactor: root.blurFactor
        onNavigateRequested: function(url) { root.navigateRequested(url) }
        onSettingsRequested: root.settingsRequested()
    }

    function openCloudLauncher() {
        cloudLauncher.openPopup()
    }

    // Wallpaper source resolution
    readonly property string wallpaperSource: {
        if (typeof browserStore !== "undefined") {
            if (browserStore.wallpaperType === "custom" && browserStore.customWallpaperPath.length > 0)
                return browserStore.fileUrl(browserStore.customWallpaperPath)
            if (browserStore.wallpaperType === "dark")
                return ""
        }
        return "qrc:/assets/cybou-alpine-dawn-1920x1080.jpg"
    }

    // Blur level resolution
    readonly property real blurFactor: {
        if (typeof browserStore !== "undefined") {
            if (browserStore.glassBlurLevel === 0) return 0.0
            if (browserStore.glassBlurLevel === 1) return 0.45
        }
        return 0.85
    }

    // Fallback base color and Deep Night gradient
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0B1D36" }
            GradientStop { position: 0.5; color: "#061324" }
            GradientStop { position: 1.0; color: "#030A14" }
        }
    }

    // Wallpaper image
    Image {
        id: bg
        anchors.fill: parent
        source: root.wallpaperSource
        visible: source.toString().length > 0
        fillMode: Image.PreserveAspectCrop
        asynchronous: false
        cache: true
        smooth: true
    }

    // Subtle atmospheric dark wash preserving sunset colors
    Rectangle {
        anchors.fill: parent
        color: "#2806172F"
        visible: bg.visible
    }

    // Central modest hero group (Floating brand, glass search bar, glass pill buttons)
    Item {
        id: centerGroup
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.height < 600 ? -8 : -18
        width: Math.min(540, Math.max(300, root.width - 48))
        implicitHeight: contentCol.implicitHeight
        opacity: root.visible ? 1.0 : 0.0
        scale: root.visible ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 250; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 250; easing.type: Easing.OutCubic } }

        ColumnLayout {
            id: contentCol
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: root.height < 600 ? 10 : 14

            Image {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: root.height < 600 ? 44 : 52
                Layout.preferredHeight: root.height < 600 ? 44 : 52
                source: "qrc:/assets/cybou.png"
                sourceSize.width: 192; sourceSize.height: 192
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "CYBOU"
                color: "#FFFFFF"
                font { pixelSize: root.height < 600 ? 26 : 32; bold: true; letterSpacing: 6 }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: root.privateMode ? qsTr("VOTRE ESPACE PRIVÉ") : qsTr("UN INTERNET À VOUS")
                color: CybouTheme.secondaryText
                font { pixelSize: 10; letterSpacing: 2.5; bold: true }
            }

            // Floating Frosted Glass Search Bar
            Item {
                Layout.fillWidth: true
                Layout.topMargin: 4
                implicitHeight: root.height < 600 ? 46 : 52

                // Ambient shadow behind search bar
                Rectangle {
                    anchors.fill: parent
                    radius: parent.height / 2
                    color: "transparent"
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: "#80000000"
                        shadowBlur: 0.6
                        shadowVerticalOffset: 6
                    }
                }

                TextField {
                    id: searchInput
                    anchors.fill: parent
                    leftPadding: 44
                    rightPadding: 20
                    placeholderText: qsTr("Où souhaitez-vous aller ?")
                    placeholderTextColor: CybouTheme.mutedText
                    font.pixelSize: 15
                    color: CybouTheme.mainText
                    selectByMouse: true
                    selectionColor: "#1D4ED8"; selectedTextColor: "#FFFFFF"

                    background: Rectangle {
                        radius: parent.height / 2
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: searchInput.activeFocus ? "#E0102646" : "#B80E2442" }
                            GradientStop { position: 1.0; color: searchInput.activeFocus ? "#F2081628" : "#C8061324" }
                        }
                        border.color: searchInput.activeFocus ? CybouTheme.borderActive : CybouTheme.borderHighlight
                        border.width: searchInput.activeFocus ? 2 : 1.5
                        Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150 } }

                        // Top specular sheen
                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                            height: 1; color: searchInput.activeFocus ? "#7038BDF8" : CybouTheme.glassSheen; radius: parent.radius
                        }
                    }

                    onAccepted: {
                        if (text.trim().length > 0) {
                            root.navigateRequested(text.trim())
                            text = ""
                        }
                    }
                }

                // Search Icon on the left
                Image {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    width: 17; height: 17
                    source: "qrc:/assets/cloud/search.svg"
                    sourceSize.width: 34; sourceSize.height: 34
                    fillMode: Image.PreserveAspectFit
                    opacity: searchInput.activeFocus ? 1.0 : 0.7
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }

            // Quick Access Glass Pill Buttons
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 2
                spacing: 12

                Button {
                    text: qsTr("Favoris")
                    onClicked: root.libraryRequested("bookmarks")
                    scale: down ? 0.95 : hovered ? 1.03 : 1.0
                    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }
                    contentItem: Text {
                        text: parent.text; color: CybouTheme.mainText
                        font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 104; implicitHeight: 36; radius: 18
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: parent.down ? "#DD0D2038" : parent.hovered ? "#DD224572" : "#9011284A" }
                            GradientStop { position: 1.0; color: parent.down ? "#EE071222" : parent.hovered ? "#EE163052" : "#A808182E" }
                        }
                        border.color: parent.hovered ? "#608BB8" : CybouTheme.borderHighlight
                        border.width: 1
                        Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                            height: 1; color: CybouTheme.glassSheenSubtle; radius: parent.radius
                        }
                    }
                }

                Button {
                    text: qsTr("Historique")
                    visible: !root.privateMode
                    onClicked: root.libraryRequested("history")
                    scale: down ? 0.95 : hovered ? 1.03 : 1.0
                    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }
                    contentItem: Text {
                        text: parent.text; color: CybouTheme.mainText
                        font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 104; implicitHeight: 36; radius: 18
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: parent.down ? "#DD0D2038" : parent.hovered ? "#DD224572" : "#9011284A" }
                            GradientStop { position: 1.0; color: parent.down ? "#EE071222" : parent.hovered ? "#EE163052" : "#A808182E" }
                        }
                        border.color: parent.hovered ? "#608BB8" : CybouTheme.borderHighlight
                        border.width: 1
                        Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                            height: 1; color: CybouTheme.glassSheenSubtle; radius: parent.radius
                        }
                    }
                }
            }

            // Private mode hint if in private mode
            Text {
                Layout.alignment: Qt.AlignHCenter
                visible: root.privateMode
                text: qsTr("Les visites privées ne sont pas enregistrées dans l’historique.")
                color: CybouTheme.mutedText
                font.pixelSize: 11
            }
        }
    }

    // Discreet bottom caption
    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        anchors.horizontalCenter: parent.horizontalCenter
        text: "CYBOU  /  " + Qt.application.version
        color: CybouTheme.mutedText
        opacity: 0.7
        font { pixelSize: 10; letterSpacing: 2 }
    }
}
