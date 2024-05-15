import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Templates as T

T.Button {
    id: root
    required property ButtonSpec spec

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    leftPadding: spec.size.leftPadding
    rightPadding: spec.size.rightPadding
    topPadding: spec.size.topPadding
    bottomPadding: spec.size.bottomPadding

    contentItem: Row {
        anchors.centerIn: root
        spacing: spec.size.iconTextSpacing
        Icon {
            anchors.verticalCenter: parent.verticalCenter
            width: spec.size.iconSize.width
            height: spec.size.iconSize.height
            source: root.icon.source
            visible: source != ""
            color: {
                if (!root.enabled)
                    return spec.style.disabledContentColor
                else if (root.hovered)
                    return spec.style.hoveredContentColor
                else
                    return spec.style.defaultContentColor
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            font.family: spec.style.fontFamily
            font.pixelSize: spec.style.fontPixelSize
            font.bold: spec.style.fontBold
            text: root.text
            color: {
                if (!root.enabled)
                    return spec.style.disabledContentColor
                else if (root.hovered)
                    return spec.style.hoveredContentColor
                else
                    return spec.style.defaultContentColor
            }
        }
    }

    background: Item {
        implicitWidth: root.implicitContentWidth + root.leftPadding + root.rightPadding
        implicitHeight: root.implicitContentHeight + root.topPadding + root.bottomPadding

        Rectangle {
            id: backgroundRect
            anchors.fill: parent
            radius: spec.style.radius
            color: {
                if (!root.enabled)
                    return spec.style.disabledBackgroundColor
                else if (root.hovered && root.enabled)
                    return spec.style.hoveredBackgroundColor
                else
                    return spec.style.defaultBackgroundColor
            }
            border.width: {
                if (root.hovered && root.enabled)
                    return spec.style.hoveredBorderWidth
                else
                    return spec.style.defaultBorderWidth
            }
            border.color: {
                if (!root.enabled)
                    return spec.style.disabledBorderColor
                else if (root.hovered)
                    return spec.style.hoveredBorderColor
                else
                    return spec.style.defaultBorderColor
            }

            visible: false
        }

        MultiEffect {
            source: backgroundRect
            anchors.fill: backgroundRect
            shadowEnabled: true
            shadowColor: backgroundRect.border.color
            shadowBlur: 0
            shadowHorizontalOffset: -1
            shadowVerticalOffset: 1
        }

        Ripple {
            anchors.fill: parent
            pressed: root.pressed
            anchor: root
            active: root.enabled && (root.down || root.visualFocus || root.hovered)
            color: spec.style.rippleColor
            clipRadius: spec.style.radius
            clip: true
        }
    }
}
