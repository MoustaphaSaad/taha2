import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    ColumnLayout {
        anchors {
            fill: parent
            margins: 8
        }

        Rectangle {
            Layout.fillWidth: true
            height: 100
            color: "blue"
        }
        TransactionsTable {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "green"
        }
    }
}
