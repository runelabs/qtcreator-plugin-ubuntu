import QtQuick 2.0
import Ubuntu.Components 0.1


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

