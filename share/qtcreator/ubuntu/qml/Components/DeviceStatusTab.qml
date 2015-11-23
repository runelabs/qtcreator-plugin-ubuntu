import QtQuick 2.4
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.DevicesModel 0.1

RowLayout {
    Controls.ScrollView {
        id: scrollView
        Layout.fillHeight: true
        Layout.minimumWidth: units.gu(78)
        ColumnLayout {
            width: scrollView.width
            Row {
                Label {
                    text:"Device Status: "+detectionStateString
                    fontSize: "large"
                }
                Item {
                    width: units.gu(2)
                    height: parent.height
                }
                ActivityIndicator{
                    visible: deviceItemView.deviceBusy
                    running: visible
                }
            }

            ListItem.SingleValue {
                text:i18n.tr("Serial ID")
                Layout.fillWidth: true
                value: serial
            }
            ListItem.SingleValue {
                text:i18n.tr("Device")
                Layout.fillWidth: true
                value: deviceInfo
            }
            ListItem.SingleValue {
                text:i18n.tr("Model")
                Layout.fillWidth: true
                value: modelInfo
            }
            ListItem.SingleValue {
                text:i18n.tr("Product")
                Layout.fillWidth: true
                value: productInfo
            }

            FeatureStateItem {
                text: "Has network connection"
                input: hasNetworkConnection
                inputRole: "hasNetworkConnection"
                Layout.fillWidth: true
                checkable: hasNetworkConnection == FeatureState.NotAvailable && !deviceItemView.deviceBusy
            }
            FeatureStateItem {
                text: "Has developer mode enabled"
                input: developerModeEnabled
                inputRole: "developerModeEnabled"
                Layout.fillWidth: true
                checkable: !deviceItemView.deviceBusy
            }
            ListItem.Divider{}

            DeviceKitManager{
                Layout.fillWidth: true
            }
        }
    }
}
