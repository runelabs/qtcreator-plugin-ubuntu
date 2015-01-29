import QtQuick 2.0
import Ubuntu.Components 1.0

import "Components"
import "DevicesPage"

MainView {
    id: modeRoot
    width: 860
    height: 548

    useDeprecatedToolbar: false
    Tabs {
        Tab {
            title: "Ubuntu Devices"
            page: DevicePage{}
        }
        Tab {
            title: "Log"
            page: LogPage{}
        }
    }
}

