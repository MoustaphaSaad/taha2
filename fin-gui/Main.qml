import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import EInk

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    DefaultButton {
        anchors.centerIn: parent
        text: "Click Me!"
        icon.source: "qrc:/qt/qml/Fin/GUI/gear.svg"
        onClicked: console.log("Click!")
    }
}
