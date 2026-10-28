import QtQuick
import QtQuick.Controls

Item {
    id: control
    required property var controller
    required property Item popupHost
    property real widthBudget: 0
    property bool editing: false
    property bool interactionsEnabled: true
    property double pendingRequestId: 0
    property var transportState: ({})
    readonly property int slotWidth: 42
    readonly property int visibleCount: icons.count
    readonly property bool overflowVisible: !editing && visibleCount < icons.count
    // While the address is being edited the eleven controls collapse into one summary button with a menu of all of
    // them; otherwise they are all shown, followed by a summary of the site's state when the row has room for it.
    readonly property real iconsWidth: (visibleCount + (overflowVisible ? 1 : 0)) * slotWidth
    // Out of editing the row keeps only its own width (plus the revoke-all button when something is granted):
    // the address takes all the rest.
    readonly property real effectiveWidth: editing ? slotWidth : iconsWidth + (summary.granted > 0 ? slotWidth : 0)
    readonly property var summary: {
        controller.revision; readyRevision
        let pending = 0, granted = 0, blocked = 0
        for (let i = 0; i < icons.count; ++i) {
            const item = icons.itemAt(i); if (!item) continue
            const c = item.capability
            if (c.pending) ++pending
            else if (c.policy === 1) ++granted
            else if (c.blocked) ++blocked
        }
        return {pending: pending, granted: granted, blocked: blocked}
    }
    function summaryText() {
        const parts = []
        if (summary.pending) parts.push(qsTr("%n en attente", "", summary.pending))
        if (summary.granted) parts.push(qsTr("%n autorisé(s)", "", summary.granted))
        if (summary.blocked) parts.push(qsTr("%n bloqué(s)", "", summary.blocked))
        return parts.length ? parts.join(" · ") : qsTr("Aucun accès en cours")
    }
    function allKeys() { return keys(0, icons.count) }
    readonly property bool hasPopup: card.visible || overflow.visible
    property int readyRevision: 0
    readonly property var visibleKeys: { controller.revision; readyRevision; return keys(0, visibleCount) }
    readonly property var hiddenKeys: { controller.revision; readyRevision; return keys(visibleCount, icons.count) }
    property string selectedKey: ""
    property var selectedIcon: null
    property var hoveredIcon: null
    property bool pinned: false
    property int rovingIndex: 0
    property var selectedData: { controller.revision; const data = controller.capability(selectedKey); return selectedKey === "network" ? Object.assign({}, data, transportState) : data }
    signal policyRequested(string key, int mode)
    signal decisionRequested(bool allow, double requestId)
    signal permissionResetRequested(string key, int mode)
    signal allowHttpRequested(string token)
    signal revokeHttpRequested(string token)
    signal allowTrackersRequested(string token)
    signal revokeTrackersRequested(string token)
    signal allowAdsRequested(string token)
    signal revokeAdsRequested(string token)
    signal reloadRequested()
    // The summary menu was used while the address had focus: leave the address so the controls expand.
    signal leaveEditingRequested()
    signal returnToEditingRequested()
    // The summary menu takes the keyboard focus from the address; editing holds while it is open.
    property bool holdEditing: false
    property bool summaryChosen: false
    signal revokeSiteRequested()
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
        if (inside) {
            hoveredIcon = icon
            closeTimer.stop()
            if (!overflow.visible) {
                if (card.visible && selectedKey !== icon.capability.key) openKey(icon.capability.key, icon, false)
                else if (!card.visible) openTimer.restart()
            }
        } else {
            if (hoveredIcon === icon) hoveredIcon = null
            openTimer.stop()
            if (!pinned) closeTimer.restart()
        }
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
    Timer { id: closeTimer; interval: 300; onTriggered: if (!control.pinned && !control.hoveredIcon && !card.pointerInside) card.close() }
    Repeater {
        id: icons
        model: control.controller.model
        delegate: ControlBarIcon {
            required property int index
            transportPending: capability.key === "network" && !!control.transportState.httpPending
            property double pressedRequestId: 0
            onPressed: pressedRequestId = control.pendingRequestId
            enabled: control.interactionsEnabled
            x: index * control.slotWidth + (control.slotWidth - 30) / 2; width: 30; height: 28
            visible: index < control.visibleCount && !control.editing
            tabStop: control.rovingIndex === index
            onActiveFocusChanged: if (activeFocus) control.rovingIndex = index
            onHoverChanged: function(inside) { control.hoverIcon(this, inside) }
            onClicked: {
                if (capability.pending) control.decisionRequested(true, pressedRequestId)
                else if (["javascript", "storage", "popups"].includes(capability.key)) control.policyRequested(capability.key, capability.policy === 1 ? 2 : 1)
                else if (["camera", "mic", "location", "notifications", "clipboard"].includes(capability.key) && (capability.policy === 1 || capability.blocked || capability.explicitDeny)) control.permissionResetRequested(capability.key, 0)
                else control.openKey(capability.key, this, true)
            }
            onFocusStep: function(direction) { control.stepFocus(index, direction) }
            Component.onCompleted: control.readyRevision++
        }
    }
    NavButton {
        id: summaryButton
        visible: control.editing
        x: 0; width: control.slotWidth; height: 28
        focusPolicy: Qt.NoFocus   // keeps the focus, and so the expanded address, while the menu is open
        icon.source: "qrc:/assets/ui/shield.svg"
        hint: qsTr("Contrôles du site : ") + control.summaryText()
        hintEnabled: !overflow.visible
        onClicked: { const point = mapToItem(control.popupHost, 0, height); overflow.x = Math.max(8, Math.min(control.popupHost.width - overflow.width - 8, point.x)); overflow.y = point.y + 4; control.holdEditing = true; control.summaryChosen = false; overflow.present(control.allKeys()) }
        Rectangle {
            visible: control.summary.pending + control.summary.granted > 0
            anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 1
            width: 15; height: 15; radius: 7.5
            color: control.summary.pending ? "#F59E0B" : "#22C55E"
            Text { anchors.centerIn: parent; textFormat: Text.PlainText; text: control.summary.pending + control.summary.granted; color: "#06121F"; font.pixelSize: 10; font.bold: true }
        }
    }
    NavButton {
        id: revokeAll
        visible: !control.editing && control.summary.granted > 0
        x: control.iconsWidth; width: control.slotWidth; height: 28
        icon.source: "qrc:/assets/ui/revoke-all.svg"
        hint: qsTr("Révoquer tous les accès de ce site") + " — " + control.summaryText()
        onClicked: control.revokeSiteRequested()
    }
    NavButton {
        id: more
        x: control.visibleCount * control.slotWidth
        width: control.slotWidth; height: 28
        visible: control.overflowVisible
        text: "···"; hint: qsTr("Autres contrôles du site")
        hintEnabled: !control.hasPopup
        focusPolicy: control.rovingIndex === control.visibleCount || activeFocus ? Qt.StrongFocus : Qt.ClickFocus
        onActiveFocusChanged: if (activeFocus) control.rovingIndex = control.visibleCount
        onClicked: { openTimer.stop(); card.close(); hoveredIcon = null; const point = mapToItem(control.popupHost, 0, height); overflow.x = Math.max(8, Math.min(control.popupHost.width - overflow.width - 8, point.x)); overflow.y = point.y + 4; overflow.present(control.hiddenKeys) }
        Keys.onLeftPressed: control.stepFocus(control.visibleCount, -1)
        Keys.onRightPressed: control.stepFocus(control.visibleCount, 1)
    }
    ControlBarOverflow {
        id: overflow; parent: control.popupHost; controller: control.controller
        onChosen: function(key) {
            if (!control.holdEditing) { control.openKey(key, more, true); return }
            control.summaryChosen = true
            control.leaveEditingRequested()
            Qt.callLater(function() { control.testOpen(key, true) })
        }
        onClosed: {
            if (!control.holdEditing) return
            control.holdEditing = false
            if (!control.summaryChosen) control.returnToEditingRequested()
        }
    }
    CapabilityPopover {
        id: card; parent: control.popupHost
        minimumY: control.mapToItem(control.popupHost, 0, control.height).y + 4
        capability: control.selectedData; pinned: control.pinned; requestId: control.pendingRequestId
        onHeightChanged: if (visible) Qt.callLater(control.placePopup)
        onOpened: Qt.callLater(control.placePopup)
        onPointerInsideChanged: if (pointerInside) closeTimer.stop(); else if (!control.pinned) closeTimer.restart()
        onClosed: { if (control.pinned && control.selectedIcon && control.selectedIcon.visible) control.selectedIcon.forceActiveFocus(); control.pinned = false }
        onPolicyRequested: function(key, mode) { control.policyRequested(key, mode) }
        onPermissionRequested: function(key, mode, targetOrigin) {
            if (mode === 0) control.permissionResetRequested(key, 0)
            else if (control.selectedData.policy === 1) control.permissionResetRequested(key, mode)
            else {
                if (control.selectedData.pending) control.decisionRequested(false, control.pendingRequestId)
                if (!controller.setPermissionPolicy(key, mode, targetOrigin || "")) control.policyRequested("error", 0)
            }
        }
        onDecisionRequested: function(allow, requestId) { control.decisionRequested(allow, requestId) }
        onReloadRequested: control.reloadRequested()
        onAllowHttpRequested: function(token) { control.allowHttpRequested(token) }
        onRevokeHttpRequested: function(token) { control.revokeHttpRequested(token) }
        onAllowTrackersRequested: function(token) { control.allowTrackersRequested(token) }
        onRevokeTrackersRequested: function(token) { control.revokeTrackersRequested(token) }
        onAllowAdsRequested: function(token) { control.allowAdsRequested(token) }
        onRevokeAdsRequested: function(token) { control.revokeAdsRequested(token) }
        onCookieRequested: browserStore.blockThirdPartyCookies = !browserStore.blockThirdPartyCookies
    }
    function testHttpAllowPoint() { return card.testHttpAllowPoint() }
    function testHttpRevokePoint() { return card.testHttpRevokePoint() }
    function testOpen(key, pin) {
        for (let i = 0; i < icons.count; ++i) { const icon = icons.itemAt(i); if (icon.capability.key === key) { openKey(key, i < visibleCount ? icon : more, pin); return true } }
        return false
    }
    function testCard() { return {key:selectedKey,pointerInside:card.pointerInside,visible:card.visible,pinned:pinned,x:card.x,y:card.y,width:card.width,height:card.height} }
    function testTrackerPoint(revoke) { return card.testTrackerPoint(revoke) }
    function testAdPoint(revoke) { return card.testAdPoint(revoke) }
    function testFocus() { if (visibleCount > 3) icons.itemAt(3).forceActiveFocus() }
    function testFocusedKey() { const icon = icons.itemAt(rovingIndex); return icon && icon.activeFocus ? icon.capability.key : "" }
    function testInputPoint(key, kind) {
        let item = key === "overflow" ? more : null
        for (let i = 0; i < visibleCount; ++i) if (icons.itemAt(i).capability.key === key) item = icons.itemAt(i)
        if (!item || !item.visible) return null
        const point = item.mapToItem(null, item.width / 2, item.height / 2)
        return {kind:kind,x:point.x,y:point.y}
    }
    function testSummary() { return {editing: editing, summaryVisible: summaryButton.visible, iconsVisible: !editing, revokeAllVisible: revokeAll.visible, text: summaryText(), width: effectiveWidth} }
    function testSummaryPoint() { const p = summaryButton.mapToItem(null, summaryButton.width / 2, summaryButton.height / 2); return {kind: "click", x: p.x, y: p.y} }
    function testOverflow() { return {visible:overflow.visible,keys:overflow.frozenKeys,currentIndex:overflow.currentIndex} }
    function testChooseHidden() { if (overflow.frozenKeys.length) { const key = overflow.frozenKeys[0]; overflow.close(); openKey(key, more, true) } }
}
