import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem


RowLayout {
    Controls.ScrollView {
        Layout.fillHeight: true
        Layout.minimumWidth: units.gu(78)
        UbuntuListView {
            model: VisualItemModel{
                Label {
                    text: "Device Control"
                    fontSize: "large"
                    anchors.left: parent.left
                }
                ListItem.Standard {
                    text:"Clone time config from Host to Device"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerCloneTimeConfig(deviceId)
                    }
                }
                ListItem.Standard {
                    text:"Enable port forwarding"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerPortForwarding(deviceId)
                    }
                }
                ListItem.Standard {
                    text:"Setup public key authentication"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerSSHSetup(deviceId)
                    }
                }
                ListItem.Standard {
                    text:"Open SSH connection to the device"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerSSHConnection(deviceId)
                    }
                }
                ListItem.Divider{}
                Label {
                    text: "Device Mode"
                    fontSize: "large"
                }
                ListItem.Standard {
                    text:"Reboot"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerReboot(deviceId)
                    }
                }
                ListItem.Standard {
                    text:"Reboot to bootloader"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerRebootBootloader(deviceId)
                    }
                }
                ListItem.Standard {
                    text:"Reboot to recovery"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerRebootRecovery(deviceId)
                    }
                }
                ListItem.Standard {
                    text:"Shutdown"
                    control: Button{
                        text: "Execute"
                        enabled: !deviceItemView.deviceBusy
                        onClicked: devicesModel.triggerShutdown(deviceId)
                    }
                }
            }
        }
    }
}

