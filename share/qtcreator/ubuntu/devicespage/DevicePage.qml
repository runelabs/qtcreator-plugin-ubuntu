import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.DevicesModel 0.1

Page {
    flickable: null
    ColumnLayout {
        anchors.fill: parent
        Controls.ToolBar {
            Layout.fillWidth: true
            height: units.gu(10)
            Row{
                anchors.fill: parent
                spacing: units.gu(2)
                Controls.ToolButton {
                    text: i18n.tr("Refresh devices")
                    tooltip: text
                    iconSource: "qrc:/ubuntu/images/view-refresh.png"
                    onClicked: devicesModel.refresh()
                }
            }
        }

        Controls.SplitView {
            orientation: Qt.Horizontal
            Layout.fillHeight: true
            Layout.fillWidth: true

            Controls.SplitView {
                orientation: Qt.Vertical
                width: 200
                Layout.minimumWidth: 200
                Layout.maximumWidth: 400

                Controls.ScrollView {
                    Layout.fillHeight: true
                    UbuntuListView {
                        id: devicesList
                        objectName: "devicesList"
                        model: devicesModel
                        delegate: ListItem.Standard {
                            id: delegate
                            text: display
                            selected: devicesList.currentIndex == index
                            onClicked: devicesList.currentIndex = index
                        }
                        onCurrentIndexChanged: deviceMode.deviceSelected(currentIndex)
                    }
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
                                DeviceStatusTab{
                                }
                            }
                            Controls.Tab {
                                title: "Advanced"
                                DeviceAdvancedTab{
                                }
                            }
                            Controls.Tab {
                                title: "Builder"
                                DeviceBuilderTab{
                                }
                            }
                            Controls.Tab {
                                title: "Log"
                                DeviceLogTab{}
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
