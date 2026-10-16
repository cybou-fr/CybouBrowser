import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    Layout.fillWidth: true
    implicitHeight: bannerVisible ? 36 : 0
    visible: implicitHeight > 0
    clip: true
    color: "#E80B1F38"

    property bool bannerVisible: false
    property int contextEpoch: -1
    property string contextTab: ""
    property string contextUrl: ""
    property string lastKey: ""
    property int lastPrevMode: 0
    property string lastOrigin: ""

    signal reloadRequested()
    signal undoRequested(string key, int prevMode)
    signal closeRequested()

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: CybouTheme.borderSubtle }
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: CybouTheme.glassSheenSubtle }

    Behavior on implicitHeight {
        NumberAnimation {
            duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 180
            easing.type: Easing.OutCubic
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        spacing: 10

        Label {
            id: policyBannerText
            Layout.fillWidth: true
            color: CybouTheme.mainText
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        GlassButton {
            text: qsTr("Recharger")
            implicitHeight: 24; padding: 4
            onClicked: {
                root.close()
                root.reloadRequested()
            }
        }

        GlassButton {
            text: qsTr("Annuler")
            implicitHeight: 24; padding: 4
            onClicked: root.undo()
        }

        NavButton {
            text: "×"
            implicitWidth: 22; implicitHeight: 22
            hint: qsTr("Fermer")
            onClicked: {
                root.close()
                root.closeRequested()
            }
        }
    }

    Timer {
        id: bannerTimer
        interval: 8000
        onTriggered: root.bannerVisible = false
    }

    function show(origin, key, mode, prevMode) {
        lastOrigin = origin
        lastKey = key
        lastPrevMode = prevMode
        const name = key === "javascript" ? "JavaScript" : key === "storage" ? qsTr("Stockage local") : qsTr("Fenêtres popups")
        const state = mode === 1 ? qsTr("activé") : qsTr("bloqué")
        policyBannerText.text = qsTr("%1 %2 pour %3. Recharger la page pour appliquer ?").arg(name).arg(state).arg(origin)
        bannerVisible = true
        bannerTimer.restart()
    }

    function undo() {
        if (lastKey.length) {
            root.undoRequested(lastKey, lastPrevMode)
        }
        close()
    }

    function close() {
        bannerVisible = false
        bannerTimer.stop()
    }
}
