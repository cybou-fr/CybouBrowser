import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    Layout.fillWidth: true
    implicitHeight: bannerVisible ? 36 : 0
    visible: implicitHeight > 0
    clip: true
    color: isRestricted ? "#E82A1406" : "#E80B1F38"

    property bool bannerVisible: false
    property bool isRestricted: false
    property int contextEpoch: -1
    property string contextTab: ""
    property string contextUrl: ""
    property string lastKey: ""
    property int lastPrevMode: 0
    property string lastOrigin: ""

    signal reloadRequested()
    signal undoRequested(string key, int prevMode)
    signal closeRequested()
    signal exitSiteRequested()
    signal enableJsRequested()

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: root.isRestricted ? "#80F59E0B" : CybouTheme.borderSubtle }
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: root.isRestricted ? "#50F59E0B" : CybouTheme.glassSheenSubtle }

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
            textFormat: Text.PlainText
            Layout.fillWidth: true
            color: root.isRestricted ? "#FFD08A" : CybouTheme.mainText
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        GlassButton {
            visible: !root.isRestricted
            text: qsTr("Recharger")
            implicitHeight: 24; padding: 4
            onClicked: {
                root.close()
                root.reloadRequested()
            }
        }

        GlassButton {
            visible: !root.isRestricted
            text: qsTr("Annuler")
            implicitHeight: 24; padding: 4
            onClicked: root.undo()
        }

        GlassButton {
            visible: root.isRestricted
            text: qsTr("Quitter le site")
            implicitHeight: 24; padding: 4
            onClicked: {
                root.exitSiteRequested()
            }
        }

        GlassButton {
            visible: root.isRestricted
            text: qsTr("Activer JavaScript")
            implicitHeight: 24; padding: 4
            onClicked: {
                root.enableJsRequested()
            }
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
        isRestricted = false
        lastOrigin = origin
        lastKey = key
        lastPrevMode = prevMode
        const name = key === "javascript" ? "JavaScript" : key === "storage" ? qsTr("Stockage local") : qsTr("Fenêtres popups")
        const state = mode === 1 ? qsTr("activé") : qsTr("bloqué")
        policyBannerText.text = qsTr("%1 %2 pour %3. Recharger la page pour appliquer ?").arg(name).arg(state).arg(origin)
        bannerVisible = true
        bannerTimer.restart()
    }

    function showRestricted(reason, tab, epoch, urlStr) {
        isRestricted = true
        bannerTimer.stop()
        contextTab = tab
        contextEpoch = epoch
        contextUrl = urlStr
        policyBannerText.text = qsTr("Mode restreint · JS désactivé · %1").arg(reason)
        bannerVisible = true
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
