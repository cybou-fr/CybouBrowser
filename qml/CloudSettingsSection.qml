import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

ColumnLayout {
    id: cloudSection
    spacing: 14
    width: parent ? parent.width : 400

    readonly property var cloudMgr: (typeof browserStore !== "undefined" && browserStore.cloud) ? browserStore.cloud : null
    readonly property bool isNextcloudInUse: {
        if (!cloudMgr || !cloudMgr.isEnabled) return false
        if (cloudMgr.defaultProvider === "nextcloud") return true
        const overrides = cloudMgr.roleOverrides
        for (let r in overrides) {
            if (overrides[r] === "nextcloud") return true
        }
        return false
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("SERVICES CLOUD & ÉCOSYSTÈME")
            color: "#38BDF8"
            font.letterSpacing: 2
            font.pixelSize: 11
            font.bold: true
            Layout.fillWidth: true
        }
        Button {
            text: qsTr("Réinitialiser tout")
            visible: cloudMgr && cloudMgr.isEnabled && Object.keys(cloudMgr.roleOverrides).length > 0
            implicitHeight: 24
            padding: 4
            contentItem: Text {
                text: parent.text
                color: "#7DD3FC"
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.hovered ? "#2038BDF8" : "transparent"
                radius: 4
                border.color: parent.hovered ? "#4038BDF8" : "transparent"
            }
            onClicked: {
                if (cloudMgr) cloudMgr.resetAllOverrides()
            }
        }
    }

    Label {
        text: qsTr("Suite cloud principale")
        color: "#E2E8F0"
        font.bold: true
    }

    ComboBox {
        id: defaultProviderCombo
        Layout.fillWidth: true
        textRole: "name"
        valueRole: "id"
        model: cloudMgr ? cloudMgr.availableProviders : []
        currentIndex: {
            if (!cloudMgr) return 0
            for (let i = 0; i < count; ++i) {
                if (model[i].id === cloudMgr.defaultProvider) return i
            }
            return 0
        }
        onActivated: {
            if (cloudMgr) {
                const selectedId = currentValue !== undefined ? currentValue : model[currentIndex].id
                cloudMgr.defaultProvider = selectedId
            }
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: "#94A3B8"
        font.pixelSize: 12
        text: cloudMgr && !cloudMgr.isEnabled
              ? qsTr("Les raccourcis cloud et le bouton d’accès rapide sont masqués sur la page d’accueil.")
              : qsTr("Définit la suite par défaut pour le lanceur d’applications (9 points) et le raccourci d’accès rapide sur la page d’accueil.")
    }

    // Nextcloud server URL configuration (shown if Nextcloud is used as default or override)
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 6
        visible: cloudSection.isNextcloudInUse

        Label {
            text: qsTr("Adresse de votre serveur Nextcloud (HTTPS)")
            color: "#E2E8F0"
            font.bold: true
            font.pixelSize: 12
        }

        TextField {
            id: ncUrlField
            Layout.fillWidth: true
            placeholderText: "https://cloud.exemple.com"
            color: "#FFFFFF"
            font.pixelSize: 13
            text: cloudMgr ? cloudMgr.nextcloudBaseUrl : ""
            onEditingFinished: {
                if (cloudMgr) {
                    cloudMgr.nextcloudBaseUrl = text
                }
            }
            background: Rectangle {
                implicitHeight: 32
                radius: 6
                color: "#102540"
                border.color: ncUrlField.activeFocus ? "#38BDF8" : "#244066"
                border.width: 1
            }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: (cloudMgr && cloudMgr.nextcloudBaseUrl.length > 0) ? "#34D399" : "#F59E0B"
            font.pixelSize: 11
            text: (cloudMgr && cloudMgr.nextcloudBaseUrl.length > 0)
                  ? qsTr("Serveur configuré : ") + cloudMgr.nextcloudBaseUrl
                  : qsTr("Veuillez renseigner l'adresse HTTPS de votre instance Nextcloud privée.")
        }
    }

    // Role-specific overrides section
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 10
        visible: cloudMgr && cloudMgr.isEnabled

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#162D4D"
        }

        Label {
            text: qsTr("PERSONNALISATION PAR SERVICE")
            color: "#64748B"
            font.letterSpacing: 1.5
            font.pixelSize: 10
            font.bold: true
        }

        Repeater {
            model: [
                { role: "mail", label: qsTr("Courrier (Mail)") },
                { role: "calendar", label: qsTr("Calendrier") },
                { role: "files", label: qsTr("Stockage & Fichiers (Drive)") },
                { role: "docs", label: qsTr("Traitement de texte (Docs)") },
                { role: "sheets", label: qsTr("Tableur (Sheets)") },
                { role: "presentations", label: qsTr("Présentations (Slides)") },
                { role: "meetings", label: qsTr("Réunions vidéo (Meet / Teams)") }
            ]

            delegate: RowLayout {
                id: roleRow
                required property var modelData
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: roleRow.modelData.label
                    color: "#CBD5E1"
                    font.pixelSize: 12
                    Layout.preferredWidth: 150
                    elide: Text.ElideRight
                }

                ComboBox {
                    id: roleCombo
                    Layout.fillWidth: true
                    textRole: "text"
                    valueRole: "value"
                    model: cloudMgr ? cloudMgr.providersForRole(roleRow.modelData.role) : []

                    currentIndex: {
                        if (!cloudMgr) return 0
                        const currentChoice = cloudMgr.roleOverride(roleRow.modelData.role)
                        for (let i = 0; i < count; ++i) {
                            if (model[i].value === currentChoice) return i
                        }
                        return 0
                    }

                    onActivated: {
                        if (cloudMgr) {
                            const val = currentValue !== undefined ? currentValue : model[currentIndex].value
                            cloudMgr.setRoleOverride(roleRow.modelData.role, val)
                        }
                    }
                }

                Button {
                    text: "↺"
                    visible: cloudMgr && cloudMgr.roleOverride(roleRow.modelData.role) !== "inherit"
                    implicitWidth: 26
                    implicitHeight: 26
                    padding: 0
                    Accessible.name: qsTr("Rétablir le choix principal")
                    contentItem: Text {
                        text: "↺"
                        color: parent.hovered ? "#38BDF8" : "#94A3B8"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#2038BDF8" : "transparent"
                        radius: 4
                    }
                    onClicked: {
                        if (cloudMgr) cloudMgr.resetRoleOverride(roleRow.modelData.role)
                    }
                }
            }
        }
    }
}
