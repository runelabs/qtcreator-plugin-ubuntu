import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

Controls.ScrollView {
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: "Platform development"
            fontSize: "large"
            anchors.left: parent.left
            Layout.fillWidth: true
        }
        Label {
            text: "The connected device can be turned  to be a native development environment.\n Use this feature with care and only if you know what are you doing."
        }
        FeatureStateItem {
            text: "Has writeable image"
            input: hasWriteableImage
            height: 24
            width: 300
        }
        FeatureStateItem {
            text: "Has device developer tools"
            input: hasDeveloperTools
            height: 24
            width: 300
        }
    }
}
