import QtQuick
import QtQuick.Effects

Item {
    id: root
    implicitWidth: img.implicitWidth
    implicitHeight: img.implicitHeight

    property alias source: img.source
    property color color: "red"

    Image {
        id: img
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        sourceSize: Qt.size(width, height)
        visible: false
    }

    MultiEffect {
        id: effect
        source: img
        anchors.fill: img
        colorization: 1
        colorizationColor: root.color
        contrast: -2
    }
}
