import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.Components.Popups 1.0

import "../Components"

Popover {
    id: popover
    height: containerLayout.childrenRect.height
    Column {
        id: containerLayout
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }
        ListItem.Header {
            text: "Ubuntu Emulator is not installed"
            __foregroundColor: UbuntuColors.orange

        }
        ListItem.Empty {
            showDivider: false
            Label {
                anchors.fill: parent
                anchors.margins: units.gu(2)
                text: "Install the emulator package \"ubuntu-emulator\" in order to create emulator instances."
                wrapMode: Text.Wrap
            }
        }
        //add a spacer item
        Item {
            height: units.gu(2)
            width:  units.gu(1)
        }
        ListItem.SingleControl {
            highlightWhenPressed: false
            control: Button {
                text: "Close"
                anchors {
                    margins: units.gu(1)
                }
                onClicked: PopupUtils.close(popover)
            }
        }
    }
}
