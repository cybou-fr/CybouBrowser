import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

Item {
    id: root
    property bool privateMode: false
    signal navigateRequested(string text)
    signal libraryRequested(string section)

    // Fallback base color before image load
    Rectangle {
        anchors.fill: parent
        color: CybouTheme.navyDeep
    }

    // Alpine dawn wallpaper
    Image {
        id: bg
        anchors.fill: parent
        source: "qrc:/assets/cybou-alpine-dawn-1920x1080.jpg"
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        smooth: true
    }

    // Subtle atmospheric dark wash preserving sunset colors
    Rectangle {
        anchors.fill: parent
        color: "#2806172F"
    }

    // Floating glass card in center
    Rectangle {
        id: card
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.height < 600 ? -10 : -25
        width: Math.min(620, Math.max(300, root.width - 48))
        implicitHeight: contentCol.implicitHeight + 48
        radius: CybouTheme.cardRadius
        color: CybouTheme.glassCard
        border.color: CybouTheme.borderHighlight
        border.width: 1.5

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
                    color: CybouTheme.glassSurface
                    border.color: searchInput.activeFocus ? CybouTheme.borderActive : CybouTheme.borderSubtle
                    border.width: searchInput.activeFocus ? 1.5 : 1
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
                    contentItem: Text {
                        text: parent.text; color: CybouTheme.mainText
                        font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 100; implicitHeight: 34; radius: CybouTheme.controlRadius
                        color: parent.hovered ? CybouTheme.glassHover : CybouTheme.glassSurface
                        border.color: CybouTheme.borderSubtle
                        border.width: 1
                    }
                }
                Button {
                    text: qsTr("Historique")
                    visible: !root.privateMode
                    onClicked: root.libraryRequested("Historique")
                    contentItem: Text {
                        text: parent.text; color: CybouTheme.mainText
                        font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 100; implicitHeight: 34; radius: CybouTheme.controlRadius
                        color: parent.hovered ? CybouTheme.glassHover : CybouTheme.glassSurface
                        border.color: CybouTheme.borderSubtle
                        border.width: 1
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
