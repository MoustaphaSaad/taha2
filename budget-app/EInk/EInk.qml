import QtQuick

QtObject {
    id: root

    property color backgroundColor: "#b1b1af"
    property color contentColor: "#292c2b"
    property color rippleColor: "#292c2b11"

    readonly property ButtonSpec defaultButtonSpec: ButtonSpec{
        size: ButtonSizeSpec {
            leftPadding: 12
            rightPadding: 12
            topPadding: 6
            bottomPadding: 6
            iconSize: Qt.size(24, 24)
            iconTextSpacing: 4
        }

        style: ButtonStyleSpec {
            fontFamily: "Roboto"
            fontPixelSize: 14
            fontBold: true
            radius: 4

            defaultContentColor: root.contentColor
            hoveredContentColor: defaultContentColor
            disabledContentColor: defaultContentColor

            defaultBackgroundColor: root.backgroundColor
            hoveredBackgroundColor: defaultBackgroundColor
            disabledBackgroundColor: defaultBackgroundColor

            defaultBorderColor: root.contentColor
            hoveredBorderColor: defaultBorderColor
            disabledBorderColor: defaultBorderColor

            rippleColor: root.rippleColor

            defaultBorderWidth: 2
            hoveredBorderWidth: defaultBorderWidth
        }
    }
}
