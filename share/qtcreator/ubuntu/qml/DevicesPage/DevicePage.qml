import QtQuick 2.4
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.3 as Controls
import QtQuick.Controls.Styles 1.3

import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.DevicesModel 0.1
import Ubuntu.Components.Popups 1.3

import "../Components"

Item {
    id: devicePage

    Item {
        visible: devicesModel.busy
        anchors.fill: parent
        anchors.topMargin: units.gu(2)
        anchors.bottomMargin: units.gu(2)
        anchors.leftMargin: units.gu(2)
        anchors.rightMargin: units.gu(4)
        RowLayout {
            //anchors.centerIn: parent
            id: busyIndicator
            anchors.left: parent.left
            anchors.top:  parent.top
            anchors.right: parent.right

            spacing: units.gu(1)
            height:  units.gu(6)

            ActivityIndicator{
                anchors.verticalCenter: parent.verticalCenter
                running: devicesModel.busy
            }
            Label {
                Layout.fillWidth: true
                anchors.verticalCenter: parent.verticalCenter
                text: i18n.tr("There is currently a process running in the background, please check the logs for details")
                fontSize: "large"
            }
            Button {
                visible: devicesModel.cancellable
                anchors.verticalCenter: parent.verticalCenter
                color: UbuntuColors.warmGrey
                text: "Cancel"
                onClicked: devicesModel.cancel()
            }
        }
        Controls.TextArea {
            id: logArea
            anchors.left: parent.left
            anchors.top:  busyIndicator.bottom
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            readOnly: true
            textFormat: TextEdit.AutoText
            Component.onCompleted: {
                deviceMode.appendText.connect(appendToLog);
            }

            function appendToLog (txt) {
                append(txt);
            }

            Connections{
                target: devicesModel
                onBusyChanged: {
                    if(!devicesModel.busy)
                        logArea.text = ""
                }
            }
        }
    }
    Controls.SplitView {
        orientation: Qt.Horizontal
        anchors.fill: parent
        visible: !devicesModel.busy
        Controls.SplitView {
            orientation: Qt.Vertical
            width: units.gu(25)
            Layout.minimumWidth: units.gu(25)
            Layout.maximumWidth: units.gu(50)

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

                        TextField{
                            id: editor
                            anchors {
                                left: parent.left
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                                margins: units.gu(1)
                            }
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
            UbuntuStyleToolbar {
                Layout.fillWidth: true
                Layout.minimumHeight: units.gu(5)
                Layout.maximumHeight: units.gu(5)

                Row{
                    anchors.fill: parent
                    spacing: units.gu(1)
                    Controls.ToolButton {
                        text: i18n.tr("Refresh devices")
                        tooltip: text
                        height: parent.height - units.gu(1)
                        width: height
                        anchors.verticalCenter: parent.verticalCenter
                        iconSource: "qrc:/ubuntu/images/reload.svg"
                        onClicked: devicesModel.refresh()
                    }
                    Controls.ToolButton {
                        text: i18n.tr("Add Emulator")
                        tooltip: text
                        iconSource: "qrc:/ubuntu/images/list-add.svg"
                        height: parent.height - units.gu(1)
                        width: height
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            if(!devicesModel.emulatorInstalled){
                                PopupUtils.open(resourceRoot+"/DevicesPage/EmulatorNotInstalled.qml",devicePage);
                                return;
                            }
                            PopupUtils.open(resourceRoot+"/DevicesPage/NewEmulatorDialog.qml",devicePage);
                        }
                    }

                    Connections{
                        target: deviceMode
                        onOpenAddEmulatorDialog: PopupUtils.open(resourceRoot+"/DevicesPage/NewEmulatorDialog.qml",devicePage);
                    }
                }
            }
        }

        Item {
            id: centerItem
            Layout.minimumWidth: units.gu(50)
            Layout.fillWidth: true

            Repeater {
                property int currentIndex: devicesList.currentIndex
                model: devicesModel

                Loader{
                    sourceComponent: deviceViewComponent
                }

                Component {
                    id: deviceViewComponent
                    Rectangle {
                        id: deviceItemView
                        property bool deviceConnected: connectionState === DeviceConnectionState.ReadyToUse || connectionState === DeviceConnectionState.Connected
                        property bool deviceBusy: (detectionState != DeviceDetectionState.Done && detectionState != DeviceDetectionState.NotStarted)
                        property bool deviceBooting: detectionState === DeviceDetectionState.Booting || detectionState === DeviceDetectionState.WaitForEmulator
                        property bool detectionError: detectionState === DeviceDetectionState.Error
                        anchors.fill: parent

                        color: Qt.rgba(0.0, 0.0, 0.0, 0.01)
                        visible: index == devicesList.currentIndex


                        UbuntuStyleToolbar {
                            id: emulatorToolBar
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            Row{
                                anchors.fill: parent
                                spacing: units.gu(1)
                                Controls.ToolButton {
                                    text: i18n.tr("Run Emulator")
                                    tooltip: text
                                    iconSource: "qrc:/projectexplorer/images/run.png"
                                    height: parent.height - units.gu(1)
                                    width: height
                                    anchors.verticalCenter: parent.verticalCenter
                                    onClicked: devicesModel.startEmulator(emulatorImageName)
                                    visible: connectionState === DeviceConnectionState.Disconnected && machineType === DeviceMachineType.Emulator
                                }
                                Controls.ToolButton {
                                    text: i18n.tr("Stop Emulator")
                                    tooltip: text
                                    iconSource: "qrc:/projectexplorer/images/stop.png"
                                    height: parent.height - units.gu(1)
                                    width: height
                                    anchors.verticalCenter: parent.verticalCenter
                                    onClicked: devicesModel.stopEmulator(emulatorImageName)
                                    visible: connectionState !== DeviceConnectionState.Disconnected && machineType === DeviceMachineType.Emulator
                                }
                                Controls.ToolButton {
                                    text: i18n.tr("Delete")
                                    tooltip: text
                                    iconSource: "qrc:/core/images/clear.png"
                                    height: parent.height - units.gu(1)
                                    width: height
                                    anchors.verticalCenter: parent.verticalCenter
                                    onClicked: {
                                        if(machineType === DeviceMachineType.Emulator)
                                            PopupUtils.open(resourceRoot+"/DevicesPage/DeleteDeviceDialog.qml",devicePage, {"emulatorImageName": emulatorImageName,"deviceId": -1 });
                                        else
                                            PopupUtils.open(resourceRoot+"/DevicesPage/DeleteDeviceDialog.qml",devicePage, {"deviceId": deviceId});
                                    }
                                    enabled: connectionState === DeviceConnectionState.Disconnected
                                }
                            }
                        }

                        ScrollableView {
                            id: deviceView
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: emulatorToolBar.bottom
                            anchors.bottom: parent.bottom
                            clip: true

                            ListItem.Empty {
                                id: errorContainer
                                divider.visible: false
                                visible: detectionError
                                height: errorText.contentHeight + units.gu(4)
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.rightMargin: units.gu(4)
                                    Image {
                                        id: errorIcon
                                        source: "qrc:/ubuntu/images/security-alert.svg"
                                        fillMode: Image.PreserveAspectFit
                                        Layout.maximumHeight: errorContainer.height
                                    }
                                    Label {
                                        id: errorText
                                        text: i18n.tr("There was a error in the device detection, check the log for details.")
                                        fontSize: "large"
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                    Button {
                                        id: deviceRedetectButton
                                        text: "Refresh"
                                        onClicked: devicesModel.triggerRedetect(deviceId)
                                    }
                                }
                            }

                            ListItem.Empty {
                                divider.visible: false
                                visible: deviceItemView.deviceBooting
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: units.gu(2)
                                    anchors.rightMargin: units.gu(4)
                                    ActivityIndicator {
                                        running: deviceItemView.deviceBooting
                                        height:parent.height - units.gu(2)
                                        width: height
                                    }
                                    Label {
                                        text: i18n.tr("The device is currently booting, if this text is still shown after the device has booted, press the refresh button.")
                                        fontSize: "large"
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                    Button {
                                        text: "Refresh"
                                        onClicked: devicesModel.triggerRedetect(deviceId)
                                    }
                                }
                            }

                            SectionItem {
                                title: deviceItemView.deviceConnected ? "Device Status: "+detectionStateString : "Device Status: Disconnected"
                                expanded: true

                                Column {
                                    anchors.left: parent.left
                                    anchors.right: parent.right

                                    ListItem.SingleValue {
                                        visible: deviceItemView.deviceConnected || machineType !== DeviceMachineType.Emulator
                                        text:i18n.tr("Serial ID")
                                        value: serial
                                    }
                                    ListItem.SingleValue {
                                        text: i18n.tr("Ubuntu version")
                                        value: emuUbuntuVersion
                                        visible: machineType === DeviceMachineType.Emulator
                                    }
                                    ListItem.SingleValue {
                                        text: i18n.tr("Device version")
                                        value: emuDeviceVersion
                                        visible: machineType === DeviceMachineType.Emulator
                                    }
                                    ListItem.SingleValue {
                                        text: i18n.tr("Image version")
                                        value: emuImageVersion
                                        visible: machineType === DeviceMachineType.Emulator
                                    }
                                    ListItem.SingleValue {
                                        text: i18n.tr("Framework version")
                                        value: frameworkVersion
                                    }
                                    ListItem.Standard {
                                        //show this listitem only when device is not connected
                                        visible: machineType === DeviceMachineType.Emulator && !deviceItemView.deviceConnected
                                        text: "Scale"
                                        control: Controls.ComboBox {
                                            id: emulatorScaleComboBox
                                            model: ["1.0", "0.9", "0.8", "0.7", "0.6","0.5", "0.4", "0.3", "0.2","0.1"]
                                            currentIndex: {
                                                var idx = find(emulatorScaleFactor);
                                                return idx >= 0 ? idx : 0;
                                            }
                                            onActivated: {
                                                emulatorScaleFactor = textAt(index);
                                            }
                                        }
                                    }

                                    ListItem.Standard {
                                        //show this listitem only when device is not connected
                                        visible: machineType === DeviceMachineType.Emulator && !deviceItemView.deviceConnected
                                        text: "Memory"
                                        control: Controls.ComboBox {
                                            id: emulatorMemoryComboBox
                                            model: ["512", "768", "1024"]

                                            currentIndex: {
                                                var idx = find(emulatorMemorySetting);
                                                return idx >= 0 ? idx : 0;
                                            }
                                            onActivated: {
                                                emulatorMemorySetting = textAt(index);
                                            }
                                        }
                                    }
                                    ListItem.SingleValue {
                                        text:i18n.tr("Device")
                                        value: deviceInfo
                                        visible: deviceItemView.deviceConnected
                                    }
                                    ListItem.SingleValue {
                                        text:i18n.tr("Model")
                                        value: modelInfo
                                        visible: deviceItemView.deviceConnected
                                    }
                                    ListItem.SingleValue {
                                        text:i18n.tr("Product")
                                        value: productInfo
                                        visible: deviceItemView.deviceConnected
                                    }

                                    FeatureStateItem {
                                        text: "Has network connection"
                                        input: hasNetworkConnection
                                        inputRole: "hasNetworkConnection"
                                        checkable: hasNetworkConnection == FeatureState.NotAvailable && !deviceItemView.deviceBusy && !deviceItemView.detectionError
                                        visible: deviceItemView.deviceConnected
                                    }
                                    FeatureStateItem {
                                        text: "Has developer mode enabled"
                                        input: developerModeEnabled
                                        inputRole: "developerModeEnabled"
                                        checkable: !deviceItemView.deviceBusy && !deviceItemView.detectionError
                                        visible: deviceItemView.deviceConnected
                                    }
                                    /*
                                    FeatureStateItem {
                                        text: "Has writeable image"
                                        input: hasWriteableImage
                                        inputRole: "hasWriteableImage"
                                        checkable: false
                                        visible: deviceItemView.deviceConnected
                                    }
                                    */
                                }
                            }

                            SectionItem {
                                title: "Kits"
                                expanded: true

                                Column {
                                    anchors.left: parent.left
                                    anchors.right: parent.right

                                    Repeater {
                                        model: kits
                                        delegate: ListItem.Standard {
                                            text: modelData.displayName
                                            Layout.fillWidth: true
                                            control: Button{
                                                text: "Remove"
                                                enabled: !deviceItemView.deviceBusy
                                                onClicked: devicesModel.triggerKitRemove(deviceId,modelData.id)
                                            }
                                        }
                                    }

                                    Item {
                                        clip: true
                                        visible: kits.length === 0
                                        height: label.contentHeight + units.gu(15)
                                        width: parent.width
                                        Label {
                                            id:label
                                            anchors.centerIn: parent
                                            anchors.bottom: button.top
                                            fontSize: "large"
                                            text: "There is currently no Kit defined for your device.\n In order to use the device in your Projects,\n you can either add a existing Kit "
                                                  +"\nor let Qt Creator autocreate one for you."
                                        }
                                        Button {
                                            id: button
                                            anchors.left: label.left
                                            anchors.right: label.right
                                            anchors.top: label.bottom
                                            anchors.bottom: parent.bottom
                                            anchors.topMargin: units.gu(2)
                                            text: "Autocreate"
                                            enabled: !deviceItemView.deviceBusy
                                            onClicked: devicesModel.triggerKitAutocreate(deviceId)
                                        }
                                    }


                                }
                            }

                            SectionItem {
                                title: "Control"
                                visible: deviceItemView.deviceConnected

                                Column {
                                    anchors.left: parent.left
                                    anchors.right: parent.right

                                    ListItem.Standard {
                                        text:"Enable port forwarding"
                                        control: Button{
                                            text: "Execute"
                                            enabled: !deviceItemView.deviceBusy && !deviceItemView.detectionError
                                            onClicked: devicesModel.triggerPortForwarding(deviceId)
                                        }
                                    }
                                    ListItem.Standard {
                                        text:"Setup public key authentication"
                                        control: Button{
                                            text: "Execute"
                                            enabled: !deviceItemView.deviceBusy && !deviceItemView.detectionError
                                            onClicked: devicesModel.triggerSSHSetup(deviceId)
                                        }
                                    }
                                    ListItem.Standard {
                                        text:"Open SSH connection to the device"
                                        control: Button{
                                            text: "Execute"
                                            enabled: !deviceItemView.deviceBusy && !deviceItemView.detectionError
                                            onClicked: devicesModel.triggerSSHConnection(deviceId)
                                        }
                                    }
                                }
                            }

                            SectionItem {
                                title: "Log"
                                Column {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    TextArea {
                                        autoSize: true
                                        maximumLineCount: 0
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: units.gu(60)
                                        highlighted: true

                                        readOnly: true
                                        text: deviceLog
                                        textFormat: TextEdit.AutoText
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

