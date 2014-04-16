import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

Column {
    ListItem.Standard {
        text:"Device Status: "+connectionStateString
    }
    FeatureStateItem {
        text: "Has devloper mode enabled"
        input: developerModeEnabled
        width: 300
    }
    FeatureStateItem {
        text: "Has network connection"
        input: hasNetworkConnection
        width: 300
    }

    Item {
        height: units.gu(5)
        width: parent.width
    }


    ListItem.Standard {
        text: "Device Kits"
        control: Button {
            text: "Add Kit"
        }

        //fontSize: "large"
        //anchors.left: parent.left
    }
    Controls.ScrollView {
        anchors.left: parent.left
        anchors.right: parent.right
        UbuntuListView {
            model: kits
            delegate: ListItem.Standard {
                text: modelData.displayName
                control: Button{
                    text: "Remove"
                }
            }
        }
    }
}

