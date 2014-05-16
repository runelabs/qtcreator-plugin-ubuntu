import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.DevicesModel 0.1
import Ubuntu.Components.Popups 0.1

Page {
    id: devicePage
    flickable: null


    Item {
        anchors.fill: parent
        visible: devicesModel.busy

        Column {
            anchors.centerIn: parent
            spacing: units.gu(1)

            ActivityIndicator{
                anchors.horizontalCenter: parent.horizontalCenter
                running: devicesModel.busy
            }
            Label {
                text: i18n.tr("There is currently a process running in the background, please check the logs for details")
                fontSize: "large"
                anchors.left: parent.left
            }
            Button {
                visible: devicesModel.cancellable
                anchors.horizontalCenter: parent.horizontalCenter
                text: "cancel"
                onClicked: devicesModel.cancel()
            }
        }
    }

    Controls.SplitView {
        orientation: Qt.Horizontal
        anchors.fill: parent
        visible: !devicesModel.busy
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
                        property alias editor: editor

                        TextField {
                            id: editor
                            anchors.fill: parent
                            visible: false

                            property bool changed: false
                            Keys.onEscapePressed: {
                                close();
                            }
                            Keys.onTabPressed: {
                                commit();
                                devicesList.incrementCurrentIndex();
                            }
                            Keys.onReturnPressed: {
                                commit();
                            }

                            onActiveFocusChanged: {
                                if(!activeFocus)
                                    close();
                            }

                            onTextChanged: {
                                changed = true;
                            }

                            function open (){
                                visible = true;
                                forceActiveFocus();
                                text = display;
                                changed = false;
                            }

                            function close (){
                                changed = false;
                                visible = false;
                            }

                            function commit (){
                                if(changed)
                                    edit = text;
                                close();
                            }

                            InverseMouseArea {
                                enabled: parent.visible
                                anchors.fill: parent
                                topmostItem: true
                                acceptedButtons: Qt.AllButtons
                                onPressed: parent.close()
                            }
                        }

                        Connections{
                            target: delegate.__mouseArea
                            onDoubleClicked: {
                                editor.open();
                            }
                        }

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
                    Controls.ToolButton {
                        text: i18n.tr("Add Emulator")
                        tooltip: text
                        iconSource: "qrc:/ubuntu/images/list-add.png"
                        onClicked: PopupUtils.open(resourceRoot+"/NewEmulatorDialog.qml",devicePage);
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
                    property bool deviceConnected: connectionState === DeviceConnectionState.ReadyToUse || connectionState === DeviceConnectionState.Connected
                    property bool deviceBusy: (detectionState != DeviceDetectionState.Done && detectionState != DeviceDetectionState.NotStarted)
                    property bool deviceBooting: detectionState === DeviceDetectionState.Booting || detectionState === DeviceDetectionState.WaitForEmulator
                    property bool detectionError: detectionState === DeviceDetectionState.Error
                    anchors.fill: parent

                    color: Qt.rgba(0.0, 0.0, 0.0, 0.01)
                    visible: index == devicesList.currentIndex

                    Controls.ToolBar {
                        id: emulatorToolBar
                        height: visible ? units.gu(5) : 0
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        visible: machineType === DeviceMachineType.Emulator
                        Row{
                            anchors.fill: parent
                            spacing: units.gu(2)
                            Controls.ToolButton {
                                text: i18n.tr("Run Emulator")
                                tooltip: text
                                iconSource: "qrc:/projectexplorer/images/run.png"
                                onClicked: devicesModel.startEmulator(emulatorImageName,emuSettings.memory)
                                visible: connectionState === DeviceConnectionState.Disconnected
                            }
                            Controls.ToolButton {
                                text: i18n.tr("Stop Emulator")
                                tooltip: text
                                iconSource: "qrc:/projectexplorer/images/stop.png"
                                onClicked: devicesModel.stopEmulator(emulatorImageName)
                                visible: connectionState !== DeviceConnectionState.Disconnected
                            }
                            Controls.ToolButton {
                                text: i18n.tr("Delete Emulator")
                                tooltip: text
                                iconSource: "qrc:/core/images/clear.png"
                                onClicked: PopupUtils.open(resourceRoot+"/DeleteEmulatorDialog.qml",devicePage, {"emulatorImageName": emulatorImageName})
                            }
                        }
                    }
                    Controls.TabView {
                        id: pagesTabView
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: emulatorToolBar.bottom
                        anchors.bottom: parent.bottom
                        anchors.margins: 12
                        visible: deviceConnected && !deviceBooting && !detectionError

                        Component.onCompleted: {
                            addTab("Device", Qt.createComponent("DeviceStatusTab.qml"))
                            if(machineType === DeviceMachineType.Emulator)
                                addTab("Emulator", Qt.createComponent("DeviceEmulatorTab.qml"))
                            addTab("Advanced", Qt.createComponent("DeviceAdvancedTab.qml"))
                            addTab("Builder", Qt.createComponent("DeviceBuilderTab.qml"))
                            addTab("Log", Qt.createComponent("DeviceLogTab.qml"))
                        }
                    }
                    Label {
                        visible: !deviceConnected && !deviceBooting && !detectionError && (machineType !== DeviceMachineType.Emulator)
                        anchors.centerIn: parent
                        text: "The device is currently not connected"
                        fontSize: "large"
                    }
                    DeviceEmulatorTab {
                        id: emuSettings
                        visible: !deviceConnected && !deviceBooting && !detectionError && (machineType === DeviceMachineType.Emulator)
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: emulatorToolBar.bottom
                        anchors.bottom: parent.bottom
                    }
                    Column {
                        visible: deviceBooting && !detectionError
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
                            running: deviceBooting
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    Column {
                        visible: detectionError
                        anchors.centerIn: parent
                        spacing: units.gu(1)
                        Label {
                            text: i18n.tr("There was a error in the device detection, please press the redetect button to try again.")
                            fontSize: "large"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Button {
                            text: "Redetect"
                            onClicked: devicesModel.triggerRedetect(deviceId)
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }
    }
}

