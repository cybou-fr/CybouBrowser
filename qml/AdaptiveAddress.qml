import QtQuick
import QtQuick.Controls

TextField {
    id: field
    property string displayHost: ""
    signal cancelEdit()
    topPadding: 0; bottomPadding: 0; leftPadding: 4; rightPadding: 4
    placeholderText: activeFocus ? qsTr("Rechercher ou saisir une adresse") : ""
    color: activeFocus ? "#0D2547" : "transparent"
    selectionColor: "#D7E4F5"; selectedTextColor: "#0D2547"
    font.pixelSize: 13; selectByMouse: true
    background: Item {}
    Accessible.name: qsTr("Adresse et recherche")
    Label {
        anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4
        visible: !field.activeFocus
        verticalAlignment: Text.AlignVCenter
        text: field.displayHost || qsTr("Rechercher ou saisir une adresse")
        textFormat: Text.PlainText; elide: Text.ElideRight
        color: field.displayHost.length ? "#0D2547" : "#738197"
        font: field.font
    }
    MouseArea {
        anchors.fill: parent; enabled: !field.activeFocus
        cursorShape: Qt.IBeamCursor
        onClicked: { field.forceActiveFocus(); field.selectAll() }
    }
}
