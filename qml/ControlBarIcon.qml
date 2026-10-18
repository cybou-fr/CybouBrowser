import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Button {
    id: icon
    required property var capability
    property bool tabStop: false
    property bool transportPending: false
    readonly property bool requestPending: !!capability.pending || transportPending
    readonly property int policy: capability.policy === undefined ? 3 : capability.policy
    readonly property bool isExplicitDeny: policy === 2 && !!capability.explicitDeny
    readonly property bool hasRecentBlock: !!capability.recent && !!capability.blocked
    readonly property color stateColor: {
        if (requestPending) return "#F59E0B"
        if (policy === 1) return "#10B981"
        if (isExplicitDeny) return "#EF4444"
        if (policy === 4) return "#F59E0B"
        if (policy === 0 && (capability.key === "network" || capability.key === "cookies")) return "#2DD4BF"
        if (policy === 3) return "#64748B"
        return "#8FA3BF"
    }
    readonly property string stateText: {
        if (requestPending) return qsTr("Demande en attente")
        if (policy === 1) return qsTr("Autorisé")
        if (isExplicitDeny) return qsTr("Bloqué")
        if (policy === 4) return qsTr("Rechargement requis")
        if (policy === 0 && (capability.key === "network" || capability.key === "cookies")) return qsTr("Filtre actif")
        if (policy === 3) return qsTr("Indisponible")
        if (hasRecentBlock) return qsTr("Tentative bloquée")
        return qsTr("Sur demande (aucun accès actif)")
    }
    signal hoverChanged(bool inside)
    signal focusStep(int direction)
    implicitWidth: 40; implicitHeight: 28
    padding: 5; hoverEnabled: true
    scale: icon.down ? 0.93 : icon.hovered ? 1.05 : 1.0
    Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100; easing.type: Easing.OutQuad } }
    focusPolicy: tabStop || activeFocus ? Qt.StrongFocus : Qt.ClickFocus
    Accessible.name: (capability.title || "") + " · " + stateText + " · " + (capability.scope || "") + " · " + (capability.reason || "") + " · " + qsTr("Gérer cet accès temporaire")
    onHoveredChanged: hoverChanged(hovered)
    Keys.onPressed: function(event) {
        if ([Qt.Key_Left, Qt.Key_Up, Qt.Key_Right, Qt.Key_Down].includes(event.key)) {
            focusStep(event.key === Qt.Key_Left || event.key === Qt.Key_Up ? -1 : 1); event.accepted = true
        }
    }
    background: Rectangle {
        radius: 7
        color: icon.down ? "#1D3B66" : icon.hovered ? "#162E4F" : (icon.policy === 1 || icon.requestPending ? Qt.rgba(icon.stateColor.r, icon.stateColor.g, icon.stateColor.b, 0.12) : (icon.isExplicitDeny ? "#20EF4444" : "transparent"))
        border.width: icon.activeFocus || (icon.policy === 1 || icon.isExplicitDeny || icon.requestPending) ? 1 : (icon.hovered ? 1 : 0)
        border.color: icon.activeFocus ? "#38BDF8" : (icon.requestPending ? "#F59E0B80" : (icon.policy === 1 ? "#5510B981" : (icon.isExplicitDeny ? "#EF444460" : (icon.hovered ? "#1E3E6B" : "transparent"))))
        Behavior on color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
        Behavior on border.color { ColorAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 120 } }
    }
    contentItem: Item {
        Image {
            id: glyph
            anchors.centerIn: parent; width: 18; height: 18
            source: "qrc:/assets/controlbar/" + icon.capability.key + ".svg"
            sourceSize.width: 36; sourceSize.height: 36
            fillMode: Image.PreserveAspectFit; visible: false
        }
        MultiEffect { anchors.fill: glyph; source: glyph; colorization: 1; colorizationColor: icon.stateColor }
    }
    Rectangle {
        visible: icon.requestPending || icon.policy === 1 || icon.policy === 4 || icon.isExplicitDeny
        x: parent.width - 12; y: 1; width: 10; height: 10; radius: icon.isExplicitDeny ? 2 : 5
        color: "#081426"; border.color: icon.stateColor; border.width: 1
        Text { textFormat: Text.PlainText; anchors.centerIn: parent; text: icon.requestPending ? "!" : icon.policy === 1 ? "✓" : icon.isExplicitDeny ? "−" : "↻"; color: icon.stateColor; font.pixelSize: 8; font.bold: true }
    }
    Rectangle {
        visible: !!icon.capability.recent && !icon.requestPending && icon.capability.key !== "network"
        x: parent.width - 9; y: parent.height - 6; width: 5; height: 5; radius: icon.capability.blocked ? 1 : 2.5
        color: icon.capability.blocked ? "#EF4444" : "#F59E0B"
    }
}
