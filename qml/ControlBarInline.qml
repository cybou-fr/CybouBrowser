import QtQuick
import QtQuick.Controls

Item {
    id: control
    required property var controller
    required property Item popupHost
    property real widthBudget: 0
    property bool editing: false
    property bool interactionsEnabled: true
    readonly property int slotWidth: 34
    readonly property int visibleCount: icons.count
    readonly property bool overflowVisible: !editing && visibleCount < icons.count
    readonly property real effectiveWidth: editing ? 0 : (visibleCount + (overflowVisible ? 1 : 0)) * slotWidth
    readonly property bool hasPopup: card.visible || overflow.visible
    property int readyRevision: 0
    readonly property var visibleKeys: { controller.revision; readyRevision; return keys(0, visibleCount) }
    readonly property var hiddenKeys: { controller.revision; readyRevision; return keys(visibleCount, icons.count) }
    property string selectedKey: ""
    property var selectedIcon: null
    property var hoveredIcon: null
    property bool pinned: false
    property int rovingIndex: 0
    property var selectedData: { controller.revision; return controller.capability(selectedKey) }
    signal policyRequested(string key, int mode)
    signal decisionRequested(bool allow)
    signal reloadRequested()
    implicitWidth: effectiveWidth; implicitHeight: 28
    function keys(first, last) {
        let result = []
        for (let i = first; i < last; ++i) { const item = icons.itemAt(i); if (item) result.push(item.capability.key) }
        return result
    }
    function dismiss() { openTimer.stop(); closeTimer.stop(); overflow.close(); card.close(); hoveredIcon = null; pinned = false; selectedKey = "" }
    function placePopup() {
        if (!selectedIcon) return
        const point = selectedIcon.mapToItem(popupHost, selectedIcon.width / 2, selectedIcon.height)
        card.width = Math.min(350, popupHost.width - 16)
        card.x = Math.max(8, Math.min(popupHost.width - card.width - 8, point.x - card.width / 2))
        card.y = Math.max(card.minimumY, Math.min(popupHost.height - card.height - 8, point.y + 4))
    }
    function openKey(key, icon, pin) {
        if (editing || !interactionsEnabled) return
        openTimer.stop(); closeTimer.stop()
        if (pin && pinned && selectedKey === key && card.visible) { dismiss(); return }
        selectedIcon = icon; selectedKey = key; pinned = pin
        placePopup(); card.open()
    }
    function hoverIcon(icon, inside) {
        if (inside) { hoveredIcon = icon; closeTimer.stop(); if (!pinned && !overflow.visible) openTimer.restart() }
        else { if (hoveredIcon === icon) hoveredIcon = null; openTimer.stop(); if (!pinned) closeTimer.restart() }
    }
    function stepFocus(index, direction) {
        const count = visibleCount + (overflowVisible ? 1 : 0)
        if (!count) return
        const next = (index + direction + count) % count
        const item = next === visibleCount ? more : icons.itemAt(next)
        if (item) item.forceActiveFocus()
    }
    onEditingChanged: if (editing) dismiss()
    onInteractionsEnabledChanged: if (!interactionsEnabled) dismiss()
    onWidthBudgetChanged: { if (card.visible) Qt.callLater(placePopup) }
    onVisibleCountChanged: { if (!overflow.visible && card.visible) dismiss(); rovingIndex = Math.min(rovingIndex, visibleCount) }
    Timer { id: openTimer; interval: 170; onTriggered: if (control.hoveredIcon) control.openKey(control.hoveredIcon.capability.key, control.hoveredIcon, false) }
    Timer { id: closeTimer; interval: 220; onTriggered: if (!control.pinned && !control.hoveredIcon && !card.pointerInside) card.close() }
    Repeater {
        id: icons
        model: control.controller.model
        delegate: ControlBarIcon {
            required property int index
            enabled: control.interactionsEnabled
            x: index * control.slotWidth; width: control.slotWidth; height: 28
            visible: index < control.visibleCount && !control.editing
            tabStop: control.rovingIndex === index
            onActiveFocusChanged: if (activeFocus) control.rovingIndex = index
            onHoverChanged: function(inside) { }
            onClicked: {
                if (capability.pending) control.decisionRequested(true)
                else if (["javascript", "storage", "popups"].includes(capability.key)) control.policyRequested(capability.key, capability.policy === 1 ? 2 : 1)
                else if (["camera", "mic", "location", "notifications", "clipboard"].includes(capability.key)) { controller.setPermissionPolicy(capability.key, 0); control.reloadRequested() }
                else control.openKey(capability.key, this, true)
            }
            onFocusStep: function(direction) { control.stepFocus(index, direction) }
            Component.onCompleted: control.readyRevision++
        }
    }
    NavButton {
        id: more
        x: control.visibleCount * control.slotWidth
        width: control.slotWidth; height: 28
        visible: control.overflowVisible
        text: "···"; hint: qsTr("Autres contrôles du site")
        ToolTip.visible: hovered && !control.hasPopup
        focusPolicy: control.rovingIndex === control.visibleCount || activeFocus ? Qt.StrongFocus : Qt.ClickFocus
        onActiveFocusChanged: if (activeFocus) control.rovingIndex = control.visibleCount
        onClicked: { openTimer.stop(); card.close(); hoveredIcon = null; const point = mapToItem(control.popupHost, 0, height); overflow.x = Math.max(8, Math.min(control.popupHost.width - overflow.width - 8, point.x)); overflow.y = point.y + 4; overflow.present(control.hiddenKeys) }
        Keys.onLeftPressed: control.stepFocus(control.visibleCount, -1)
        Keys.onRightPressed: control.stepFocus(control.visibleCount, 1)
    }
    ControlBarOverflow {
        id: overflow; parent: control.popupHost; controller: control.controller
        onChosen: function(key) { control.openKey(key, more, true) }
    }
    CapabilityPopover {
        id: card; parent: control.popupHost
        minimumY: control.mapToItem(control.popupHost, 0, control.height).y + 4
        capability: control.selectedData; pinned: control.pinned
        onHeightChanged: if (visible) Qt.callLater(control.placePopup)
        onOpened: Qt.callLater(control.placePopup)
        onPointerInsideChanged: if (pointerInside) closeTimer.stop(); else if (!control.pinned) closeTimer.restart()
        onClosed: { if (control.pinned && control.selectedIcon && control.selectedIcon.visible) control.selectedIcon.forceActiveFocus(); control.pinned = false }
        onPolicyRequested: function(key, mode) { control.policyRequested(key, mode) }
        onPermissionRequested: function(key, mode) { if (!controller.setPermissionPolicy(key, mode)) control.policyRequested("error", 0) }
        onDecisionRequested: function(allow) { control.decisionRequested(allow) }
        onReloadRequested: control.reloadRequested()
        onCookieRequested: browserStore.blockThirdPartyCookies = !browserStore.blockThirdPartyCookies
    }
    function testOpen(key, pin) {
        for (let i = 0; i < icons.count; ++i) { const icon = icons.itemAt(i); if (icon.capability.key === key) { openKey(key, i < visibleCount ? icon : more, pin); return true } }
        return false
    }
    function testCard() { return {visible:card.visible,pinned:pinned,x:card.x,y:card.y,width:card.width,height:card.height} }
    function testFocus() { if (visibleCount > 3) icons.itemAt(3).forceActiveFocus() }
    function testFocusedKey() { const icon = icons.itemAt(rovingIndex); return icon && icon.activeFocus ? icon.capability.key : "" }
    function testInputPoint(key, kind) {
        let item = key === "overflow" ? more : null
        for (let i = 0; i < visibleCount; ++i) if (icons.itemAt(i).capability.key === key) item = icons.itemAt(i)
        if (!item || !item.visible) return null
        const point = item.mapToItem(null, item.width / 2, item.height / 2)
        return {kind:kind,x:point.x,y:point.y}
    }
    function testOverflow() { return {visible:overflow.visible,keys:overflow.frozenKeys,currentIndex:overflow.currentIndex} }
    function testChooseHidden() { if (overflow.frozenKeys.length) { const key = overflow.frozenKeys[0]; overflow.close(); openKey(key, more, true) } }
}
