import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels

Rectangle {
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.left: tableView.left
        anchors.top: parent.top
        syncView: tableView
        model: ["Amount", "Source", "Target", "Date"]
        clip: true
    }
    TableView {
        id: tableView
        anchors.top: horizontalHeader.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        columnSpacing: 1
        rowSpacing: 1
        clip: true

        model: TableModel {
            TableModelColumn {
                display: "Amount"
            }
            TableModelColumn {
                display: "Source"
            }
            TableModelColumn {
                display: "Target"
            }
            TableModelColumn {
                display: "Date"
            }

            rows: [{
                    "Amount": "36",
                    "Source": "mostafa",
                    "Target": "tarnsport-uber",
                    "Date": "10, May, 2024",
                }, {
                    "Amount": "300",
                    "Source": "mostafa",
                    "Target": "gov-paperwork",
                    "Date": "10, May, 2024",
                }, {
                    "Amount": "532",
                    "Source": "mostafa",
                    "Target": "subscription-kagi",
                    "Date": "9, May, 2024",
                }, {
                    "Amount": "30",
                    "Source": "mostafa",
                    "Target": "subscription-amazon",
                    "Date": "6, May, 2024",
                }]
        }

        delegate: Rectangle {
            implicitWidth: text.implicitWidth + 80
            implicitHeight: text.implicitHeight + 20
            border.width: 1

            Text {
                id: text
                text: display
                anchors.centerIn: parent
            }
        }
    }
}
