import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.Components.Popups 0.1

Page {
    flickable: null
    id: myPage

    tools: ToolbarItems {
        ToolbarButton {
            action: Action {
                text: i18n.tr("Add Emulator")
                iconSource: "qrc:/ubuntu/images/list-add.png"
                onTriggered: PopupUtils.open(resourceRoot+"/NewEmulatorDialog.qml",myPage);
            }
        }
        ToolbarButton {
            action: Action {
                text: i18n.tr("Refresh emulators")
                iconSource: "qrc:/ubuntu/images/view-refresh.png"
                onTriggered: emulatorModel.findEmulatorImages()
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: emulatorModel.busy

        Column {
            anchors.centerIn: parent
            spacing: units.gu(1)

            ActivityIndicator{
                anchors.horizontalCenter: parent.horizontalCenter
                running: emulatorModel.busy
            }
            Label {
                text: i18n.tr("There is currently a process running in the background, please check the logs for details")
                fontSize: "large"
                anchors.left: parent.left
            }
            Button {
                visible: emulatorModel.cancellable
                anchors.horizontalCenter: parent.horizontalCenter
                text: "cancel"
                onClicked: emulatorModel.cancel()
            }
        }
    }

    Controls.SplitView {
        orientation: Qt.Horizontal
        anchors.fill: parent
        visible: !emulatorModel.busy

        Controls.ScrollView {
            width: 200
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.maximumWidth: 400
            UbuntuListView {
                id: emulatorList
                objectName: "emulatorList"
                model: emulatorModel
                delegate: ListItem.Standard {
                    id: delegate
                    text: display
                    selected: emulatorList.currentIndex == index
                    onClicked: emulatorList.currentIndex = index
                }
                onCurrentIndexChanged: deviceMode.deviceSelected(currentIndex)
            }
        }
        Item {
            id: centerItem
            Layout.minimumWidth: 400
            Layout.fillWidth: true
            property int currentIndex: emulatorList.currentIndex
            Repeater {
                model: emulatorModel
                anchors.fill: parent
                Rectangle{
                    id: deviceItemView
                    anchors.fill: parent
                    anchors.margins: 12

                    color: Qt.rgba(0.0, 0.0, 0.0, 0.01)
                    visible: index == emulatorList.currentIndex && !emulatorModel.busy

                    UbuntuListView {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        height: units.gu(50)
                        width: units.gu(50)
                        model: VisualItemModel {
                            ListItem.SingleValue {
                                text: i18n.tr("Ubuntu version")
                                value: ubuntuVersion
                            }
                            ListItem.SingleValue {
                                text: i18n.tr("Device version")
                                value: deviceVersion
                            }
                            ListItem.SingleValue {
                                text: i18n.tr("Image version")
                                value: imageVersion
                            }
                            ListItem.SingleControl {
                                control: Button {
                                    text: "Start emulator"
                                    onClicked: emulatorModel.startEmulator(display)
                                }
                            }
                            /*
                                ListItem.SingleControl {
                                    control: Button {
                                        text: "Delete emulator"
                                    }
                                }
                                */
                        }
                    }

                    Label {
                        visible: emulatorModel.busy
                        anchors.centerIn: parent
                        text: emulatorModel.state
                        fontSize: "large"
                    }
                }
            }
        }
    }
}
