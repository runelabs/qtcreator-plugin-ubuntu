import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.Components.Popups 0.1

RowLayout {
    UbuntuListView {
        anchors.left: parent.left
        width: units.gu(50)
        model: VisualItemModel {
            ListItem.SingleValue {
                text: i18n.tr("Ubuntu version")
                //value: ubuntuVersion
            }
            ListItem.SingleValue {
                text: i18n.tr("Device version")
                //value: deviceVersion
            }
            ListItem.SingleValue {
                text: i18n.tr("Image version")
                //value: imageVersion
            }

        }
    }
}
