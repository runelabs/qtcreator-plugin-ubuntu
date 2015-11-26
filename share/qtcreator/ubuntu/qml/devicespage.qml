import QtQuick 2.4
import Ubuntu.Components 1.3

import "Components"
import "DevicesPage"

MainView {
    id: modeRoot
    width: 860
    height: 548

    Page {
        header: PageHeader {
            id: header
            title: "Ubuntu Devices"
            sections.model: ["Devices", "Log"]
        }

        DevicePage{
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            visible: header.sections.selectedIndex == 0
        }
        LogPage{
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            visible: header.sections.selectedIndex == 1
        }
    }
}

