import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.DevicesModel 0.1

Page {
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
                    objectName: "devicesList"
                    model: devicesModel
                    currentIndex: 0
                    delegate: ListItem.Standard {
                        id: delegate
                        text: display
                        selected: devicesList.currentIndex == index
                        onClicked: devicesList.currentIndex = index
                    }
                    onCurrentIndexChanged: deviceMode.deviceSelected(currentIndex)
                }
            }
            Controls.ToolBar {
                Layout.fillWidth: true
                Layout.minimumHeight: units.gu(5)
                Layout.maximumHeight: units.gu(5)
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
                    property bool deviceBooting: detectionState === States.Booting
                    anchors.fill: parent
                    anchors.margins: 12

                    color: Qt.rgba(0.0, 0.0, 0.0, 0.01)
                    visible: index == devicesList.currentIndex

                    Controls.TabView {
                        id: pagesTabView
                        anchors.fill: parent
                        visible: (connectionState === States.DeviceReadyToUse || connectionState === States.DeviceConnected) && !deviceBooting
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
                        visible: !pagesTabView.visible && !deviceBooting
                        anchors.centerIn: parent
                        text: "The device is currently not connected"
                        fontSize: "large"
                    }
                    Column {
                        visible: !pagesTabView.visible && deviceBooting
                        anchors.centerIn: parent
                        spacing: units.gu(1)
                        Label {
                            text: i18n.tr("The device is currently booting.")
                            fontSize: "large"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Label {
                            text: i18n.tr("If this text is still shown after the device has booted, press the refresh button.")
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        ActivityIndicator {
                            running: connectionState === States.Booting
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }
    }
}

