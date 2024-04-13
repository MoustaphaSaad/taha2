import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import EInk

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Rectangle {
        id: sourceItem
        anchors.centerIn: parent
        color: "#b1b1af"
        radius: 8
        width: 200
        height: 36
        border.width: 2
        border.color: "#292c2b"
        visible: false

        Text {
            anchors.centerIn: parent
            text: "Click Me!"
            color: "#292c2b"
            font.pixelSize: 16
            font.weight: Font.Bold
        }
    }

    MultiEffect {
        source: sourceItem
        anchors.fill: sourceItem
        shadowEnabled: true
        shadowColor: "#292c2b"
        shadowBlur: 0
        autoPaddingEnabled: true
        paddingRect: Qt.rect(20, 20, 40, 30)
        shadowHorizontalOffset: -2
        shadowVerticalOffset: 2
    }
}