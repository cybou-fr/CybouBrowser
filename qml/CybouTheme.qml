pragma Singleton
import QtQuick

QtObject {
    // Approved CYBOU 0.8 design tokens
    readonly property color navyBase: "#0D2547"
    readonly property color navyDeep: "#06172F"
    readonly property color navySurface: "#0A182F"
    readonly property color glassSurface: "#D0122C50"
    readonly property color glassCard: "#EA081528"
    readonly property color glassHover: "#603C608B"
    readonly property color borderHighlight: "#497DA5CC"
    readonly property color borderSubtle: "#1E3E6B"
    readonly property color borderActive: "#0088FF"
    readonly property color mainText: "#F1F7FF"
    readonly property color secondaryText: "#B9CBE2"
    readonly property color mutedText: "#64748B"
    readonly property color brandRed: "#E11D2E"
    readonly property color allow: "#31E3A3"
    readonly property color deny: "#FF6077"
    readonly property color request: "#FFBD5C"
    readonly property color recent: "#76B9FF"
    readonly property color neutral: "#9CB2CE"
    readonly property color unavailable: "#687C99"

    // Glass specular highlights and reflections
    readonly property color glassSheen: "#70FFFFFF"
    readonly property color glassSheenSubtle: "#35FFFFFF"
    readonly property color glassCardTop: "#750E2648"
    readonly property color glassCardBottom: "#B0061224"

    // Geometry constants
    readonly property int chromeRowHeight: 36
    readonly property int chromeControlHeight: 28
    readonly property int tabRadius: 8
    readonly property int controlRadius: 8
    readonly property int cardRadius: 16
    readonly property int dialogRadius: 16
}
