import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.DevicesModel 0.1

ColumnLayout {
    ListItem.Standard {
        Layout.fillWidth: true
        text:"Device Status: "+detectionStateString
        control: ActivityIndicator{
            visible: deviceItemView.deviceBusy
            running: true
        }
    }
    FeatureStateItem {
        text: "Has network connection"
        input: hasNetworkConnection
        inputRole: "hasNetworkConnection"
        width: 300
        checkable: hasNetworkConnection != States.Available
    }
    FeatureStateItem {
        text: "Has devloper mode enabled"
        input: developerModeEnabled
        inputRole: "developerModeEnabled"
        width: 300
        checkable: !deviceItemView.deviceBusy
    }
    Item {
        height: units.gu(5)
        Layout.fillWidth: true
    }

    ListItem.Standard {
        text: "Device Kits"
        control: Button {
            text: "Add Kit"
            enabled: !deviceItemView.deviceBusy
        }
        Layout.fillWidth: true
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Controls.ScrollView {
            visible: kits.length > 0
            anchors.fill: parent
            UbuntuListView {
                model: kits
                delegate: ListItem.Standard {
                    text: modelData.displayName
                    control: Button{
                        text: "Remove"
                        enabled: !deviceItemView.deviceBusy
                    }
                }
            }
        }

        Item {
            clip: true
            visible: kits.length === 0
            anchors.fill: parent
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
                anchors.topMargin: units.gu(2)
                text: "Autocreate"
                enabled: !deviceItemView.deviceBusy
                onClicked: devicesModel.triggerKitAutocreate(deviceId)
            }
        }
    }
}

