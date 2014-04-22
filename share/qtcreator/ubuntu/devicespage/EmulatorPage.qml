import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

Page {
    flickable: null
    tools: ToolbarItems {
        ToolbarButton {
            action: Action {
                iconSource: "qrc:/ubuntu/images/ubuntu-64.png"
                text: "add emulator"
            }
        }
        Button {
            anchors.verticalCenter: parent.verticalCenter
            text: "add emulator"
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Controls.ToolBar {
            Layout.fillWidth: true
            RowLayout {
                Controls.ToolButton {
                    iconSource: "qrc:/ubuntu/images/ubuntu-64.png"
                    text: "Add emulator"
                    tooltip: text
                }
            }
        }

        Controls.SplitView {
            orientation: Qt.Horizontal
            Layout.fillWidth: true
            Layout.fillHeight: true

            Controls.SplitView {
                orientation: Qt.Vertical
                width: 200
                Layout.fillHeight: true
                Layout.minimumWidth: 200
                Layout.maximumWidth: 400

                Button {
                    Layout.maximumHeight: implicitHeight
                    Layout.minimumHeight: implicitHeight
                    text: "Add new emulator"
                }
                Controls.ScrollView {
                    Layout.fillHeight: true
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
                Button {
                    Layout.maximumHeight: implicitHeight
                    Layout.minimumHeight: implicitHeight
                    text: "Add new emulator"
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
                            anchors.fill: parent
                            model: VisualItemModel {
                                ListItem.SingleControl {
                                    control: Button {
                                        text: "Start emulator"

                                    }
                                }
                                ListItem.SingleControl {
                                    control: Button {
                                        text: "Delete emulator"
                                    }
                                }
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
}
