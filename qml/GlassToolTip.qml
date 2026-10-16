import QtQuick
import QtQuick.Controls

ToolTip {
    id: tip
    objectName: "cybouGlassTooltip"
    popupType: Popup.Item
    delay: 500
    padding: 9
    horizontalPadding: 12
    implicitWidth: Math.min(340, implicitContentWidth + leftPadding + rightPadding)
    contentItem: Text {
        text: tip.text
        textFormat: Text.PlainText
        color: CybouTheme.mainText
        font.pixelSize: 12
        wrapMode: Text.Wrap
    }
    background: Rectangle {
        radius: 8
        gradient: Gradient {
            GradientStop { position: 0; color: "#ED183654" }
            GradientStop { position: 1; color: "#F00B1D35" }
        }
        border.width: 1
        border.color: CybouTheme.borderHighlight
    }
}
