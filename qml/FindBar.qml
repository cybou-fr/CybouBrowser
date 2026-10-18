import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    Layout.fillWidth: true
    implicitHeight: visible ? 48 : 0
    visible: false
    color: "#0A182F"

    property alias text: findInput.text
    property string resultText: ""

    signal findNext(string text)
    signal findPrevious(string text)
    signal closeRequested()

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#11243E" }

    RowLayout {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 12
        spacing: 8

        TextField {
            id: findInput
            placeholderText: qsTr("Rechercher dans la page")
            color: "white"
            placeholderTextColor: "#64748B"
            background: Rectangle {
                radius: 6; color: "#0B1E38"; border.color: "#1B3B66"
            }
            onAccepted: root.findNext(text)
        }

        Label {
            textFormat: Text.PlainText
            text: root.resultText
            color: "#8FA3BF"
        }

        NavButton {
            text: "↑"
            hint: qsTr("Précédent")
            onClicked: root.findPrevious(findInput.text)
        }

        NavButton {
            text: "↓"
            hint: qsTr("Suivant")
            onClicked: root.findNext(findInput.text)
        }

        NavButton {
            text: "×"
            hint: qsTr("Fermer la recherche")
            onClicked: root.closeRequested()
        }
    }

    function focusInput() {
        findInput.forceActiveFocus()
        findInput.selectAll()
    }
}
