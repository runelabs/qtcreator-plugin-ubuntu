import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.DevicesModel 0.1

Item {
    ColumnLayout {
        anchors.fill: parent
        Label {
            text: "Device Kits"
            fontSize: "large"
        }

        Column {
            Layout.fillWidth: true
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
        }

        Item {
            clip: true
            visible: kits.length === 0
            Layout.minimumHeight: units.gu(25)
            Layout.fillWidth: true
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

        //Spacer Item
        Item {
            Layout.fillHeight: true
        }
    }
}
