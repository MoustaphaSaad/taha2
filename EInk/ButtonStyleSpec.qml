import QtQuick

QtObject {
    required property string fontFamily
    required property int fontPixelSize
    required property bool fontBold

    required property int radius

    required property color defaultContentColor
    required property color hoveredContentColor
    required property color disabledContentColor

    required property color defaultBackgroundColor
    required property color hoveredBackgroundColor
    required property color disabledBackgroundColor

    required property color defaultBorderColor
    required property color hoveredBorderColor
    required property color disabledBorderColor

    required property color rippleColor

    required property double defaultBorderWidth
    required property double hoveredBorderWidth
}
