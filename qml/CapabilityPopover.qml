import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Cybou

Popup {
    id: card
    property var capability: ({})
    property bool pinned: false
    property double requestId: 0
    property bool pointerInside: hover.hovered || surfaceHover.hovered
    property real minimumY: 0
    signal policyRequested(string key, int mode)
    signal permissionRequested(string key, int mode, string targetOrigin)
    signal decisionRequested(bool allow, double requestId)
    signal reloadRequested()
    signal cookieRequested()
    signal allowHttpRequested(string token)
    signal revokeHttpRequested(string token)
    signal allowTrackersRequested(string token)
    signal revokeTrackersRequested(string token)
    signal allowAdsRequested(string token)
    signal revokeAdsRequested(string token)
    property string networkFilter: "all"
    onVisibleChanged: if (!visible) networkFilter = "all"

    component GlassBtn: Button {
        id: gbtn
        property double pressedRequestId: 0
        property string pressedHttpToken: ""
        onPressed: { pressedRequestId = card.requestId; pressedHttpToken = card.capability.httpToken || "" }
        scale: down ? 0.95 : hovered ? 1.02 : 1.0
        Behavior on scale { NumberAnimation { duration: (typeof browserStore !== "undefined" && browserStore.reduceMotion) ? 0 : 100 } }
        contentItem: Text {
            textFormat: Text.PlainText
            text: gbtn.text; color: gbtn.highlighted ? "#FFFFFF" : gbtn.enabled ? CybouTheme.mainText : CybouTheme.mutedText
            font.pixelSize: 12; font.bold: gbtn.highlighted || gbtn.hovered
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 6
            gradient: Gradient {
                GradientStop { position: 0.0; color: gbtn.highlighted ? "#E0183A68" : gbtn.down ? "#DD0D2038" : gbtn.hovered ? "#DD224572" : "#80142F52" }
                GradientStop { position: 1.0; color: gbtn.highlighted ? "#E80E2444" : gbtn.down ? "#EE071222" : gbtn.hovered ? "#EE163052" : "#900B1D35" }
            }
            border.color: gbtn.highlighted ? CybouTheme.borderActive : gbtn.hovered ? "#7DA5CC" : CybouTheme.borderHighlight
            border.width: 1
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                height: 1; color: gbtn.highlighted ? "#50FFFFFF" : CybouTheme.glassSheenSubtle; radius: parent.radius
            }
        }
    }
    function testHttpAllowPoint() { const p = httpAllow.mapToItem(null, httpAllow.width/2, httpAllow.height/2); return {kind:"click",x:p.x,y:p.y} }
    function testHttpRevokePoint() { const p = httpRevoke.mapToItem(null, httpRevoke.width/2, httpRevoke.height/2); return {kind:"click",x:p.x,y:p.y} }
    function testTrackerPoint(revoke) { const button = revoke ? trackerRevoke : trackerAllow; const p = button.mapToItem(null,button.width/2,button.height/2); return {kind:"click",x:p.x,y:p.y} }
    function testAdPoint(revoke) { const button = revoke ? adRevoke : adAllow; const p = button.mapToItem(null,button.width/2,button.height/2); return {kind:"click",x:p.x,y:p.y} }
    width: 330; padding: 16
    height: Math.min(implicitHeight, parent.height - minimumY - 8)
    focus: pinned; modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        HoverHandler { id: surfaceHover }
        radius: 16
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#D81B3554" }
            GradientStop { position: 1.0; color: "#EB081629" }
        }
        border.color: "#497DA5CC"
        border.width: 1.5
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#80000000"
            shadowBlur: 1.0
            shadowVerticalOffset: 12
        }
        Rectangle {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
            height: 1; color: "#50FFFFFF"; radius: parent.radius
        }
    }
    contentItem: ScrollView {
        implicitHeight: content.implicitHeight
        contentWidth: availableWidth
        clip: true
        HoverHandler { id: hover }
        ColumnLayout {
        id: content
        width: parent.width
        spacing: 10
        RowLayout {
            spacing: 10
            Rectangle {
                width: 36; height: 36; radius: 8
                color: "#122540"; border.color: "#304D75"; border.width: 1
                Image {
                    id: popGlyph
                    anchors.centerIn: parent; width: 20; height: 20
                    source: card.capability.key ? "qrc:/assets/controlbar/" + card.capability.key + ".svg" : ""
                    sourceSize.width: 36; sourceSize.height: 36
                    fillMode: Image.PreserveAspectFit; visible: false
                }
                MultiEffect {
                    anchors.fill: popGlyph; source: popGlyph; colorization: 1
                    colorizationColor: {
                        switch (card.capability.key) {
                        case "camera": return "#38BDF8"
                        case "mic": return "#10B981"
                        case "location": return "#38BDF8"
                        case "notifications": return "#F59E0B"
                        case "javascript": return "#10B981"
                        case "popups": return "#FB923C"
                        case "cookies": return "#C084FC"
                        case "storage": return "#818CF8"
                        case "clipboard": return "#38BDF8"
                        case "network": return "#2DD4BF"
                        default: return "#94A3B8"
                        }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 1
                Label { textFormat: Text.PlainText; text: card.capability.title || ""; color: "white"; font.bold: true; font.pixelSize: 15 }
                Label { text: card.capability.origin || qsTr("Page locale"); textFormat: Text.PlainText; color: "#7DD3FC"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
            }
            Button {
                text: "×"; implicitWidth: 26; implicitHeight: 26; onClicked: card.close(); Accessible.name: qsTr("Fermer les détails")
                contentItem: Text { text: "×"; color: parent.hovered ? "#FFFFFF" : "#94A3B8"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { radius: 6; color: parent.hovered ? "#E11D2E" : "#142948" }
            }
        }
        Label {
            textFormat: Text.PlainText
            Layout.fillWidth: true
            text: card.capability.pending ? qsTr("Demande d’accès reçue")
                : card.capability.key === "network" ? qsTr("Observation du profil")
                : card.capability.key === "cookies" ? qsTr("Politique du profil")
                : card.capability.policy === 1 ? qsTr("Autorisé")
                : (card.capability.policy === 2 && card.capability.explicitDeny) ? qsTr("Bloqué pour les prochaines demandes")
                : card.capability.policy === 4 ? qsTr("Recharger pour appliquer")
                : card.capability.policy === 3 ? qsTr("Fonction indisponible")
                : qsTr("Sur demande (aucun accès actif)")
            color: "#FFFFFF"; wrapMode: Text.WordWrap; font.bold: true
        }
        Label { textFormat: Text.PlainText; Layout.fillWidth: true; text: card.capability.reason || ""; color: "#CBD5E1"; font.pixelSize: 12; wrapMode: Text.WordWrap }
        Label { visible: !!card.capability.requester && card.capability.requester !== card.capability.origin; Layout.fillWidth: true; text: qsTr("Origine demandeuse : ") + (card.capability.requester || ""); textFormat: Text.PlainText; color: "#FFD08A"; font.pixelSize: 11; wrapMode: Text.WrapAnywhere }
        RowLayout {
            visible: !!card.capability.pending
            GlassBtn { text: qsTr("Autoriser cette demande"); onClicked: card.decisionRequested(true, pressedRequestId) }
            GlassBtn { text: qsTr("Refuser"); onClicked: card.decisionRequested(false, pressedRequestId) }
        }
        Flow {
            Layout.fillWidth: true; spacing: 5
            visible: !!card.capability.canAct && ["camera", "mic", "location", "notifications", "clipboard"].includes(card.capability.key)
            GlassBtn {
                text: ["camera", "mic"].includes(card.capability.key) ? qsTr("Révoquer l’accès") : qsTr("Réinitialiser")
                onClicked: card.permissionRequested(card.capability.key, 0, "")
            }
            GlassBtn { text: qsTr("Bloquer"); onClicked: card.permissionRequested(card.capability.key, 2, "") }
            GlassBtn {
                visible: !!card.capability.requester && card.capability.requester !== card.capability.origin
                text: qsTr("Bloquer le cadre")
                onClicked: card.permissionRequested(card.capability.key, 2, card.capability.requester)
            }
        }
        Flow {
            Layout.fillWidth: true; spacing: 5
            visible: !!card.capability.canAct && ["javascript", "storage", "popups"].includes(card.capability.key)
            GlassBtn { text: qsTr("Autoriser"); onClicked: card.policyRequested(card.capability.key, 1) }
            GlassBtn { text: qsTr("Bloquer"); onClicked: card.policyRequested(card.capability.key, 2) }
            GlassBtn { visible: card.capability.key === "popups"; text: qsTr("Gestes explicites"); onClicked: card.policyRequested(card.capability.key, 0) }
        }
        Label {
            visible: ["camera", "mic"].includes(card.capability.key)
            Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#C4D6EB"; font.pixelSize: 11
            text: qsTr("La révocation coupe immédiatement les flux média sans détruire la page ni recharger l’onglet.")
        }
        GlassBtn { visible: card.capability.policy === 4 || (!!card.capability.canAct && ["camera", "mic"].includes(card.capability.key)); text: qsTr("Recharger l’onglet"); onClicked: card.reloadRequested() }
        GlassBtn { visible: card.capability.key === "cookies"; text: browserStore.blockThirdPartyCookies ? qsTr("Autoriser les cookies tiers du profil") : qsTr("Bloquer les cookies tiers du profil"); onClicked: card.cookieRequested() }
        ColumnLayout {
            visible: card.capability.key === "network"
            Layout.fillWidth: true; spacing: 4
            Label {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; textFormat: Text.PlainText
                color: "#FFD08A"
                visible: !!card.capability.httpPending || !!card.capability.httpTemporary
                text: card.capability.httpTemporary ? qsTr("HTTP temporaire actif pour cette page · profil réservé à cet onglet. Cookies et cache en mémoire.")
                    : qsTr("Connexion HTTP bloquée : %1. Ouvrir cette adresse dans un profil isolé pour cette page ? Les données ne seront pas chiffrées. Cookies et cache séparés.").arg(card.capability.httpOrigin || "")
            }
            GlassBtn {
                id: httpAllow
                visible: !!card.capability.httpPending
                text: qsTr("Ouvrir cette adresse HTTP")
                onClicked: card.allowHttpRequested(pressedHttpToken)
            }
            GlassBtn {
                id: httpRevoke
                visible: !!card.capability.httpTemporary
                text: qsTr("Révoquer HTTP et fermer la page")
                onClicked: card.revokeHttpRequested(pressedHttpToken)
            }
            Label {
                textFormat: Text.PlainText
                text: qsTr("Requêtes filtrées · profils ouverts : %1").arg(browserStore.totalBlockedTrackers())
                color: "#38BDF8"; font.pixelSize: 12; font.bold: true
            }
            Label {
                textFormat: Text.PlainText
                text: qsTr("Échantillon réseau récent : %1").arg(card.capability.sampleCount || 0)
                color: "#A9C1DC"; font.pixelSize: 11
            }
            Label {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#B9CBE2"; font.pixelSize: 11
                text: qsTr("Bloqués · HTTP : %1 · pubs : %2 · traqueurs : %3 · autres : %4\nPassés par exception : %5")
                    .arg((card.capability.networkCounts || {}).transport || 0)
                    .arg((card.capability.networkCounts || {}).ads || 0)
                    .arg((card.capability.networkCounts || {}).trackers || 0)
                    .arg((card.capability.networkCounts || {}).other || 0)
                    .arg((card.capability.networkCounts || {}).exceptions || 0)
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#224A75" }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                Label {
                    textFormat: Text.PlainText
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#FFD08A"
                    text: card.capability.adTemporary ? qsTr("Exception publicitaire active pour cette page. Révoquée au prochain chargement.")
                        : qsTr("Publicités bloquées par défaut. Si le contenu est altéré, vous pouvez autoriser temporairement les annonces.")
                }
                RowLayout {
                    spacing: 6
                    GlassBtn {
                        id: adAllow
                        visible: !card.capability.adTemporary
                        enabled: !!card.capability.adCanAllow
                        text: qsTr("Autoriser les publicités")
                        onClicked: card.allowAdsRequested(pressedHttpToken)
                    }
                    GlassBtn {
                        id: adRevoke
                        visible: !!card.capability.adTemporary
                        text: qsTr("Révoquer les publicités et recharger")
                        onClicked: card.revokeAdsRequested(pressedHttpToken)
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#224A75" }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                Label {
                    textFormat: Text.PlainText
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#FFD08A"
                    text: card.capability.trackerTemporary ? qsTr("Exception de traqueurs active pour cette page. Révoquée au prochain chargement.")
                        : qsTr("Pistage bloqué par défaut. Si le site est cassé, vous pouvez autoriser temporairement les outils de mesure.")
                }
                RowLayout {
                    spacing: 6
                    GlassBtn {
                        id: trackerAllow
                        visible: !card.capability.trackerTemporary
                        enabled: !!card.capability.trackerCanAllow
                        text: qsTr("Autoriser les traqueurs")
                        onClicked: card.allowTrackersRequested(pressedHttpToken)
                    }
                    GlassBtn {
                        id: trackerRevoke
                        visible: !!card.capability.trackerTemporary
                        text: qsTr("Révoquer les traqueurs et recharger")
                        onClicked: card.revokeTrackersRequested(pressedHttpToken)
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#224A75" }
            Flow {
                Layout.fillWidth: true; spacing: 4
                GlassBtn { text: qsTr("Tout"); highlighted: card.networkFilter === "all"; onClicked: card.networkFilter = "all" }
                GlassBtn { text: qsTr("HTTP"); highlighted: card.networkFilter === "transport"; onClicked: card.networkFilter = "transport" }
                GlassBtn { text: qsTr("Pubs"); highlighted: card.networkFilter === "ads"; onClicked: card.networkFilter = "ads" }
                GlassBtn { text: qsTr("Traqueurs"); highlighted: card.networkFilter === "trackers"; onClicked: card.networkFilter = "trackers" }
                GlassBtn { text: qsTr("Autres"); highlighted: card.networkFilter === "other"; onClicked: card.networkFilter = "other" }
            }
        }
        Label { visible: !!card.capability.recent && card.capability.key !== "network"; Layout.fillWidth: true; text: qsTr("Événements Qt récents (60 s), pas une preuve d’utilisation active."); color: "#A9C1DC"; font.pixelSize: 11; wrapMode: Text.WordWrap }
        Repeater {
            model: (card.capability.events || []).filter(function(event) { return card.capability.key !== "network" || card.networkFilter === "all" || event.category === card.networkFilter })
            delegate: Label {
                required property var modelData
                Layout.fillWidth: true
                text: new Date(modelData.time).toLocaleTimeString(Qt.locale(), "HH:mm:ss")
                    + (modelData.scope ? "  [" + (modelData.scope === "tab" ? qsTr("Onglet") : qsTr("Profil")) + "]  " : "  ")
                    + modelData.origin
                    + (modelData.resourceLabel ? " · " + modelData.resourceLabel : "")
                    + (modelData.blocked ? " · " + qsTr("Bloqué") : modelData.resourceType !== undefined ? " · " + qsTr("Passé par le filtre") : "")
                    + (modelData.explanation ? "\n" + modelData.explanation : "")
                textFormat: Text.PlainText; color: "#BBD2EA"; font.pixelSize: 10; wrapMode: Text.WrapAnywhere
            }
        }
        }
    }
}
