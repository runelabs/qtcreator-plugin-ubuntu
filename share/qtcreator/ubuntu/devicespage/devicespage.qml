import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1

Item {
    SplitView {
        orientation: Qt.Horizontal
        anchors.fill: parent

        ScrollView {
            width: 200
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.maximumWidth: 400

            ListView {
                id: devicesList
                model: devicesModel
                delegate: Item {
                    id: delegate
                    width:parent.width
                    height: theTxt.height+2
                    Row {
                        Text{
                            id: theTxt
                            text: display
                            clip: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        Item {
            id: centerItem
            Layout.minimumWidth: 50
            Layout.fillWidth: true
        }
    }
}

