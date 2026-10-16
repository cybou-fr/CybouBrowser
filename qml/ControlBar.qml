import QtQuick
import QtQuick.Controls

Item {
    id: control
    required property var controller
    property string position: browserStore.controlBarPosition
    readonly property bool vertical: position === "left" || position === "right"
    property string selectedKey: ""
    property var selectedIcon: null
    property var hoveredIcon: null
    property bool pinned: false
    property var selectedData: { controller.revision; return controller.capability(selectedKey) }
    signal policyRequested(string key, int mode)
    signal decisionRequested(bool allow)
    signal reloadRequested()
    z: 100
    function dismiss() { card.close(); picker.close(); selectedKey = ""; pinned = false; openTimer.stop(); closeTimer.stop() }
    function openIcon(icon, pin) {
        closeTimer.stop(); openTimer.stop()
        if (pin && pinned && selectedKey === icon.capability.key && card.visible) { dismiss(); return }
        pinned = pin; selectedIcon = icon; selectedKey = icon.capability.key
        if (selectedKey === "position") {
            card.close(); placePopup(picker, icon); picker.open(); return
        }
        picker.close(); placePopup(card, icon); card.open()
    }
    function placePopup(popup, icon) {
        const point = icon.mapToItem(control, 0, 0)
        const pw = Math.min(popup.width, control.width - 16)
        popup.width = pw
        const ph = Math.min(popup.height || popup.implicitHeight || 360, control.height - 16)
        popup.x = Math.max(8, Math.min(control.width - pw - 8, position === "right" ? point.x - pw - 8 : position === "left" ? point.x + icon.width + 8 : point.x - pw / 2 + icon.width / 2))
        popup.y = Math.max(8, Math.min(control.height - ph - 8, position === "bottom" ? point.y - ph - 8 : position === "top" ? point.y + icon.height + 8 : point.y))
    }
    function hoverIcon(icon, inside) {
        if (inside) { hoveredIcon = icon; closeTimer.stop(); if (!pinned && icon.capability.key !== "position") openTimer.restart() }
        else { if (hoveredIcon === icon) hoveredIcon = null; openTimer.stop(); if (!pinned) closeTimer.restart() }
    }
    function stepFocus(index, direction) {
        const target = (index + direction + 12) % 12
        const icon = target === 11 ? positionIcon : iconRepeater.itemAt(target)
        if (icon) icon.forceActiveFocus()
    }
    onPositionChanged: dismiss()
    onSelectedDataChanged: if (card.visible && selectedIcon) Qt.callLater(placePopup, card, selectedIcon)
    onVisibleChanged: if (!visible) dismiss()
    Timer { id: openTimer; interval: 170; onTriggered: if (hoveredIcon) control.openIcon(hoveredIcon, false) }
    Timer { id: closeTimer; interval: 220; onTriggered: if (!control.pinned && !control.hoveredIcon && !card.pointerInside) card.close() }
    Rectangle {
        id: rail
        width: control.vertical ? 36 : Math.min(control.width - 56, 36 + 11 * 32)
        height: control.vertical ? Math.min(control.height - 16, 36 + 11 * 32) : 36
        x: control.position === "left" ? 8 : control.position === "right" ? control.width - width - 28 : (control.width - width - 20) / 2
        y: control.position === "bottom" ? control.height - height - 8 : 8
        radius: 18; color: "#AA0D2547"; border.color: "#55D7E7FF"; border.width: 1
        Flickable {
            id: strip
            x: 2; y: 2
            width: control.vertical ? 32 : parent.width - 36
            height: control.vertical ? parent.height - 36 : 32
            contentWidth: icons.implicitWidth; contentHeight: icons.implicitHeight
            clip: true; boundsBehavior: Flickable.StopAtBounds
            flickableDirection: control.vertical ? Flickable.VerticalFlick : Flickable.HorizontalFlick
            WheelHandler {
                onWheel: function(event) {
                    if (control.vertical) strip.contentY = Math.max(0, Math.min(strip.contentHeight - strip.height, strip.contentY - event.angleDelta.y / 3))
                    else strip.contentX = Math.max(0, Math.min(strip.contentWidth - strip.width, strip.contentX - event.angleDelta.y / 3))
                    event.accepted = true
                }
            }
            Grid {
                id: icons
                columns: control.vertical ? 1 : 11; spacing: 0
                Repeater {
                    id: iconRepeater
                    model: control.controller.model
                    delegate: ControlBarIcon {
                        required property int index
                        vertical: control.vertical; separator: [6,8].includes(index)
                        onHoverChanged: function(inside) { control.hoverIcon(this, inside) }
                        onClicked: control.openIcon(this, true)
                        onFocusStep: function(direction) { control.stepFocus(index, direction) }
                    }
                }
            }
        }
        ControlBarIcon {
            id: positionIcon
            x: control.vertical ? 2 : rail.width - 34
            y: control.vertical ? rail.height - 34 : 2
            vertical: control.vertical; separator: true
            capability: ({key:"position", title:qsTr("Position du panneau"),scope:qsTr("Préférence globale"),reason:qsTr("Choisir haut, droite, bas ou gauche"),policy:0})
            onClicked: control.openIcon(this, true)
            onFocusStep: function(direction) { control.stepFocus(11, direction) }
            ToolTip.visible: hovered; ToolTip.text: qsTr("Position du panneau")
        }
    }
    CapabilityPopover {
        id: card
        parent: control
        capability: control.selectedData
        pinned: control.pinned
        onHeightChanged: if (visible && control.selectedIcon) Qt.callLater(control.placePopup, card, control.selectedIcon)
        onOpened: if (control.selectedIcon) Qt.callLater(control.placePopup, card, control.selectedIcon)
        onPointerInsideChanged: if (pointerInside) closeTimer.stop(); else if (!control.pinned) closeTimer.restart()
        onClosed: { if (control.pinned && control.selectedIcon) control.selectedIcon.forceActiveFocus(); control.pinned = false }
        onPolicyRequested: function(key, mode) { control.policyRequested(key, mode) }
        onPermissionRequested: function(key, mode) { if (!controller.setPermissionPolicy(key, mode)) control.policyRequested("error", 0) }
        onDecisionRequested: function(allow) { control.decisionRequested(allow) }
        onReloadRequested: control.reloadRequested()
        onCookieRequested: browserStore.blockThirdPartyCookies = !browserStore.blockThirdPartyCookies
    }
    ControlBarPositionPicker { id: picker; parent: control }
    function testGeometry() { return {x:rail.x,y:rail.y,width:rail.width,height:rail.height,viewportWidth:width,viewportHeight:height} }
    function testOpen(key, pin) { for (let i = 0; i < 11; ++i) { const icon = iconRepeater.itemAt(i); if (icon && icon.capability.key === key) { openIcon(icon, pin); return true } } return false }
    function testCard() { return {visible:card.visible,pinned:pinned,x:card.x,y:card.y,width:card.width,height:card.height} }
    function testInputPoint(key, kind) {
        for (let i = 0; i < 11; ++i) {
            const icon = iconRepeater.itemAt(i)
            if (icon && icon.capability.key === key) { const point = icon.mapToItem(null, 16, 16); return {kind:kind,x:point.x,y:point.y} }
        }
        return null
    }
}
