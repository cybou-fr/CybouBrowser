import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtWebEngine
import Cybou

Drawer {
    id: library
    property var browserWindow: null
    property var settingsPanelRef: null
    property string librarySection: "bookmarks"
    property var downloadItems: []

    signal openUrlRequested(string url)
    signal reviewDownloadRequested(var download)
    signal clearHistoryRequested()

    function showSection(section) {
        if (section === "Favoris" || section === "bookmarks") librarySection = "bookmarks"
        else if (section === "Historique" || section === "history") librarySection = "history"
        else if (section === "Téléchargements" || section === "downloads") librarySection = "downloads"
        else librarySection = section || "bookmarks"
        open()
    }

    parent: Overlay.overlay
    edge: Qt.RightEdge
    width: Math.min(460, (browserWindow ? browserWindow.width : 1360) - 60)
    y: browserWindow ? browserWindow.contentItem.y : 0
    height: browserWindow ? browserWindow.contentItem.height : 800
    modal: false
    dim: false

    onAboutToShow: if (settingsPanelRef) settingsPanelRef.close()

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#D80A1F38" }
            GradientStop { position: 1.0; color: "#E8061224" }
        }
        border.color: CybouTheme.borderHighlight
        border.width: 1
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#90000000"
            shadowBlur: 1.0
            shadowHorizontalOffset: -12
        }
        Rectangle {
            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            width: 1; color: CybouTheme.glassSheen
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 14
        RowLayout {
            Label {
                textFormat: Text.PlainText
                text: librarySection === "bookmarks" ? qsTr("Favoris") : librarySection === "history" ? qsTr("Historique") : qsTr("Téléchargements")
                font.pixelSize: 23; font.bold: true; color: CybouTheme.mainText; Layout.fillWidth: true
            }
            // Section actions as icons with tooltips, next to closing the panel.
            NavButton { icon.source: "qrc:/assets/ui/clear-history.svg"; hint: qsTr("Effacer l’historique"); visible: librarySection === "history"; onClicked: library.clearHistoryRequested() }
            NavButton { icon.source: "qrc:/assets/ui/downloads-folder.svg"; hint: qsTr("Ouvrir le dossier Téléchargements"); visible: librarySection === "downloads"; onClicked: browserStore.openDownloads() }
            NavButton { text: "×"; hint: qsTr("Fermer"); onClicked: library.close() }
        }
        RowLayout {
            spacing: 6
            NavButton { icon.source: "qrc:/assets/ui/bookmarks.svg"; hint: qsTr("Favoris"); highlighted: librarySection === "bookmarks"; implicitWidth: 38; implicitHeight: 32; onClicked: librarySection = "bookmarks" }
            NavButton { icon.source: "qrc:/assets/ui/history.svg"; hint: qsTr("Historique"); highlighted: librarySection === "history"; implicitWidth: 38; implicitHeight: 32; onClicked: librarySection = "history" }
            NavButton { icon.source: "qrc:/assets/ui/files.svg"; hint: qsTr("Fichiers"); highlighted: librarySection === "downloads"; implicitWidth: 38; implicitHeight: 32; onClicked: librarySection = "downloads" }
        }
        TextField {
            id: librarySearch; Layout.fillWidth: true; placeholderText: qsTr("Filtrer…")
            visible: librarySection !== "downloads"
            color: CybouTheme.mainText; placeholderTextColor: CybouTheme.mutedText
            background: Rectangle {
                radius: 8
                gradient: Gradient {
                    GradientStop { position: 0.0; color: librarySearch.activeFocus ? "#E0143258" : "#C0102644" }
                    GradientStop { position: 1.0; color: librarySearch.activeFocus ? "#F00A1A30" : "#D008172A" }
                }
                border.color: librarySearch.activeFocus ? CybouTheme.borderActive : CybouTheme.borderHighlight
                border.width: 1
            }
        }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 8
            model: librarySection === "bookmarks" ? browserStore.bookmarks : librarySection === "history" ? browserStore.history : downloadItems
            delegate: Rectangle {
                id: entry
                required property var modelData
                property bool download: librarySection === "downloads"
                property bool matches: download || (modelData.title + " " + modelData.url).toLowerCase().includes(librarySearch.text.toLowerCase())
                width: ListView.view.width; height: matches ? (download ? 92 : 64) : 0; visible: matches
                radius: 10
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#60142F52" }
                    GradientStop { position: 1.0; color: "#700A182E" }
                }
                border.color: CybouTheme.borderSubtle
                border.width: 1
                Rectangle {
                    anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 1
                    height: 1; color: "#25FFFFFF"; radius: parent.radius
                }
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 4
                    RowLayout {
                        spacing: 2
                        Label { textFormat: Text.PlainText; Layout.fillWidth: true; text: entry.download ? entry.modelData.downloadFileName : entry.modelData.title; color: CybouTheme.mainText; elide: Text.ElideRight; font.bold: true }
                        NavButton {
                            icon.source: "qrc:/assets/ui/open.svg"; hint: entry.download ? qsTr("Ouvrir le fichier") : qsTr("Ouvrir")
                            visible: !entry.download || entry.modelData.state === WebEngineDownloadRequest.DownloadCompleted
                            onClicked: {
                                if (entry.download) library.reviewDownloadRequested(entry.modelData)
                                else { library.openUrlRequested(entry.modelData.url); library.close() }
                            }
                        }
                        NavButton { icon.source: "qrc:/assets/ui/folder.svg"; hint: qsTr("Afficher dans le dossier"); visible: entry.download; onClicked: browserStore.showFileFolder(browserWindow ? browserWindow.downloadPath(entry.modelData) : "") }
                        NavButton {
                            visible: entry.download && !entry.modelData.isFinished
                            icon.source: entry.download && entry.modelData.isPaused ? "qrc:/assets/ui/resume.svg" : "qrc:/assets/ui/pause.svg"
                            hint: entry.download && entry.modelData.isPaused ? qsTr("Reprendre") : qsTr("Pause")
                            enabled: entry.download && entry.modelData.state === WebEngineDownloadRequest.DownloadInProgress
                            onClicked: entry.modelData.isPaused ? entry.modelData.resume() : entry.modelData.pause()
                        }
                        NavButton { icon.source: "qrc:/assets/ui/cancel.svg"; hint: qsTr("Annuler le téléchargement"); visible: entry.download && !entry.modelData.isFinished; onClicked: entry.modelData.cancel() }
                        NavButton { icon.source: "qrc:/assets/ui/remove.svg"; hint: qsTr("Retirer des favoris"); visible: librarySection === "bookmarks"; onClicked: browserStore.toggleBookmark(entry.modelData.url, entry.modelData.title) }
                    }
                    Label { textFormat: Text.PlainText; Layout.fillWidth: true; text: entry.download ? (browserWindow ? browserWindow.downloadStatus(entry.modelData) : "") : entry.modelData.url; elide: Text.ElideRight; color: CybouTheme.secondaryText; font.pixelSize: 11 }
                    ProgressBar { visible: entry.download; Layout.fillWidth: true; from: 0; to: entry.download ? Math.max(1, entry.modelData.totalBytes) : 1; value: entry.download ? entry.modelData.receivedBytes : 0; indeterminate: entry.download && entry.modelData.totalBytes <= 0 && !entry.modelData.isFinished }
                }
            }
            Label { anchors.centerIn: parent; visible: parent.count === 0; text: qsTr("Rien ici pour le moment"); color: "#8491A5" }
        }
    }
}
