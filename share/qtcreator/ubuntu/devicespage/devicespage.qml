import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.DevicesModel 0.1

MainView {

    width: 860
    height: 548
    Page {
        title: "Ubuntu Devices"
        flickable: null
            Controls.SplitView {
                orientation: Qt.Horizontal
                anchors.fill: parent

                Controls.SplitView {
                    orientation: Qt.Vertical
                    width: 200
                    Layout.minimumWidth: 200
                    Layout.maximumWidth: 400

                    Controls.ScrollView {
                        Layout.fillHeight: true
                        UbuntuListView {
                            id: devicesList
                            model: devicesModel
                            delegate: ListItem.Standard {
                                id: delegate
                                text: display
                                selected: devicesList.currentIndex == index
                                onClicked: devicesList.currentIndex = index
                            }
                        }
                    }
                    Button {
                        Layout.maximumHeight: implicitHeight
                        Layout.minimumHeight: implicitHeight
                        text: "Create new emulator"
                    }
                }
                Item {
                    id: centerItem
                    Layout.minimumWidth: 400
                    Layout.fillWidth: true
                    property int currentIndex: devicesList.currentIndex
                    Repeater {
                        model: devicesModel
                        anchors.fill: parent
                        Rectangle{
                            id: deviceItemView
                            property bool deviceBusy: (detectionState != States.Done && detectionState != States.NotStarted)
                            anchors.fill: parent
                            anchors.margins: 12

                            color: Qt.rgba(0.0, 0.0, 0.0, 0.01)
                            visible: index == devicesList.currentIndex

                            Controls.TabView {
                                id: pagesTabView
                                anchors.fill: parent
                                visible: connectionState === States.DeviceReadyToUse || connectionState === States.DeviceConnected
                                Controls.Tab {
                                    title: "Device"
                                    StatusPage{
                                    }
                                }
                                Controls.Tab {
                                    title: "Advanced"
                                    AdvancedPage{
                                    }
                                }
                                Controls.Tab {
                                    title: "Builder"
                                    BuilderPage{
                                    }
                                }
                                Controls.Tab {
                                    title: "Log"
                                    LogPage{}
                                }
                            }
                            Label {
                                visible: !pagesTabView.visible
                                anchors.centerIn: parent
                                text: "The device is currently not connected"
                                fontSize: "large"
                            }
                        }
                    }
                }
            }

    }
}

