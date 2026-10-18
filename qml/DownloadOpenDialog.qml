import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Cybou

Dialog {
    id: dialog
    property string pendingOpenPath: ""
    property string pendingOpenOrigin: ""
    signal statusMessage(string message)

    anchors.centerIn: parent
    width: Math.min(500, (parent ? parent.width : 600) - 40)
    modal: true
    title: qsTr("Ouvrir un fichier téléchargé")
    padding: 20
    background: GlassDialogBackground {}
    header: Label {
        textFormat: Text.PlainText
        text: dialog.title; padding: 20; bottomPadding: 8
        color: CybouTheme.mainText; font.pixelSize: 18; font.bold: true; wrapMode: Text.WordWrap
    }
    property bool allowed: visible && browserStore.canOpenDownloadedFile(pendingOpenPath)

    contentItem: ColumnLayout {
        spacing: 14
        Label {
            Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText
            font.bold: true; color: CybouTheme.mainText
            text: browserStore.fileName(browserStore.fileUrl(pendingOpenPath))
        }
        Label {
            Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText
            color: CybouTheme.secondaryText
            text: qsTr("Source : %1").arg(pendingOpenOrigin || qsTr("Source inconnue"))
        }
        Label {
            textFormat: Text.PlainText
            Layout.fillWidth: true; wrapMode: Text.WordWrap; color: CybouTheme.mainText
            text: dialog.allowed ? qsTr("Ce fichier sera ouvert par une application externe. Continuez uniquement si vous faites confiance à sa source. CYBOU ne réalise pas d’analyse antivirus.") : qsTr("L’ouverture de ce type de fichier est bloquée dans CYBOU. Les programmes, scripts, raccourcis et types inconnus doivent être examinés depuis leur dossier.")
        }
        Button {
            text: qsTr("Afficher le dossier")
            onClicked: { browserStore.showFileFolder(pendingOpenPath); dialog.reject() }
        }
    }
    footer: DialogButtonBox {
        background: Rectangle { color: "transparent"; radius: CybouTheme.dialogRadius }
        Button {
            text: qsTr("Annuler"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            focus: true; onClicked: dialog.reject()
        }
        Button {
            text: qsTr("Ouvrir dans une application"); enabled: dialog.allowed
            onClicked: {
                const path = pendingOpenPath
                dialog.accept()
                if (!browserStore.openDownloadedFile(path))
                    dialog.statusMessage(qsTr("Impossible d’ouvrir ce fichier en toute sécurité."))
            }
        }
    }
    onClosed: { pendingOpenPath = ""; pendingOpenOrigin = "" }
}
