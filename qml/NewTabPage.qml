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

    // Ambient drop shadow beneath card
    Rectangle {
        id: cardShadow
        anchors.centerIn: card
        anchors.verticalCenterOffset: 14
        width: card.width - 20
        height: card.height - 16
        radius: CybouTheme.cardRadius + 4
        color: "#A0000000"
        layer.enabled: true
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1.0
            blurMax: 32
        }
    }

    // Floating frosted glass card in center
    Rectangle {
        id: card
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.height < 600 ? -10 : -25
        width: Math.min(620, Math.max(300, root.width - 48))
        implicitHeight: contentCol.implicitHeight + (root.height < 600 ? 36 : 48)
        radius: CybouTheme.cardRadius
        clip: true
        color: "transparent"
        opacity: root.visible ? 1.0 : 0.0
        scale: root.visible ? 1.0 : 0.96
        Behavior on opacity { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 250; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 250; easing.type: Easing.OutCubic } }

        // Frosted window: aligned portion of blurred wallpaper
        Item {
            anchors.fill: parent
            visible: bg.visible && root.blurFactor > 0.01
            layer.enabled: true
            layer.effect: MultiEffect {
                blurEnabled: true
                blur: root.blurFactor
                blurMax: 40
            }

            ShaderEffectSource {
                x: -card.x
                y: -card.y
                width: root.width
                height: root.height
                sourceItem: bg
                live: true
            }
        }

        // Tinted glass gradient wash
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: CybouTheme.glassCardTop }
                GradientStop { position: 1.0; color: CybouTheme.glassCardBottom }
            }
        }

        // Top specular highlight line
        Rectangle {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            height: 1; color: CybouTheme.glassSheen
        }

        // Crisp glass border
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.color: CybouTheme.borderHighlight
            border.width: 1.5
        }

        ColumnLayout {
            id: contentCol
            anchors.fill: parent
            anchors.margins: root.height < 600 ? 18 : 24
            spacing: root.height < 600 ? 12 : 16

            Image {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: root.height < 600 ? 64 : 84
                Layout.preferredHeight: root.height < 600 ? 64 : 84
                source: "qrc:/assets/cybou.png"
                sourceSize.width: 256; sourceSize.height: 256
                fillMode: Image.PreserveAspectFit
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "CYBOU"
                color: CybouTheme.mainText
                font { pixelSize: root.height < 600 ? 32 : 42; bold: true; letterSpacing: 8 }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: root.privateMode ? qsTr("VOTRE ESPACE PRIVÉ") : qsTr("UN INTERNET À VOUS")
                color: CybouTheme.secondaryText
                font { pixelSize: 11; letterSpacing: 3; bold: true }
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 96; implicitHeight: 3
                color: "transparent"
                Row {
                    anchors.centerIn: parent; spacing: 0
                    Rectangle { width: 32; height: 3; color: "#2563EB"; radius: 1 }
                    Rectangle { width: 32; height: 3; color: "#FFFFFF"; radius: 1 }
                    Rectangle { width: 32; height: 3; color: CybouTheme.brandRed; radius: 1 }
                }
            }

            TextField {
                id: searchInput
                Layout.fillWidth: true
                Layout.topMargin: 4
                implicitHeight: root.height < 600 ? 46 : 52
                leftPadding: 20; rightPadding: 20
                placeholderText: qsTr("Où souhaitez-vous aller ?")
                placeholderTextColor: CybouTheme.mutedText
                font.pixelSize: 15
                color: CybouTheme.mainText
                selectByMouse: true
                selectionColor: "#1D4ED8"; selectedTextColor: "#FFFFFF"
                background: Rectangle {
                    radius: CybouTheme.controlRadius + 4
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: searchInput.activeFocus ? "#E0143258" : "#C0102644" }
                        GradientStop { position: 1.0; color: searchInput.activeFocus ? "#F00A1A30" : "#D008172A" }
                    }
                    border.color: searchInput.activeFocus ? CybouTheme.borderActive : CybouTheme.borderHighlight
                    border.width: searchInput.activeFocus ? 2 : 1
                    Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 150 } }
                    Rectangle {
                        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                        height: 1; color: searchInput.activeFocus ? "#500088FF" : CybouTheme.glassSheenSubtle; radius: parent.radius
                    }
                }
                onAccepted: {
                    if (text.trim().length > 0) {
                        root.navigateRequested(text.trim())
                        text = ""
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12
                Button {
                    text: qsTr("Favoris")
                    onClicked: root.libraryRequested("Favoris")
                    scale: down ? 0.95 : hovered ? 1.03 : 1.0
                    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }
                    contentItem: Text {
                        text: parent.text; color: CybouTheme.mainText
                        font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 104; implicitHeight: 36; radius: CybouTheme.controlRadius
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: parent.down ? "#DD0D2038" : parent.hovered ? "#DD224572" : "#C0142F52" }
                            GradientStop { position: 1.0; color: parent.down ? "#EE071222" : parent.hovered ? "#EE163052" : "#D00B1D35" }
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
                    onClicked: root.libraryRequested("Historique")
                    scale: down ? 0.95 : hovered ? 1.03 : 1.0
                    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }
                    contentItem: Text {
                        text: parent.text; color: CybouTheme.mainText
                        font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 104; implicitHeight: 36; radius: CybouTheme.controlRadius
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: parent.down ? "#DD0D2038" : parent.hovered ? "#DD224572" : "#C0142F52" }
                            GradientStop { position: 1.0; color: parent.down ? "#EE071222" : parent.hovered ? "#EE163052" : "#D00B1D35" }
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

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: root.privateMode ? qsTr("Les visites privées ne sont pas enregistrées dans l’historique.") : qsTr("Un nouvel horizon. Un espace à vous.")
                color: CybouTheme.mutedText
                font.pixelSize: 11
            }
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter
            text: "CYBOU  /  ÉDITION 0.8"
            color: CybouTheme.mutedText
            font { pixelSize: 10; letterSpacing: 2 }
        }
    }
}
