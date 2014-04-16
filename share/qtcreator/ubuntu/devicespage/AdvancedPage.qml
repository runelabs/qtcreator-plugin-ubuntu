import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

Controls.ScrollView {
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: "Device Control"
            fontSize: "large"
            anchors.left: parent.left
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Clone time config from Host to Device"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Enable port forwarding"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Setup public key authentication"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Open SSH connection to the device"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
        ListItem.Divider{}
        Label {
            text: "Device Mode"
            fontSize: "large"
            anchors.left: parent.left
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Reboot"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Reboot to bootloader"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Reboot to recovery"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
        ListItem.Standard {
            text:"Shutdown"
            control: Button{
                text: "Execute"
            }
            Layout.fillWidth: true
        }
    }
}
