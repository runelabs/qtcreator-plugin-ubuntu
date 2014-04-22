import QtQuick 2.0
import Ubuntu.Components 0.1


MainView {

    width: 860
    height: 548

    Tabs {
        Tab {
            title: "Ubuntu Devices"
            page: DevicePage{}
        }
        Tab {
            title: "Emulators"
            page: EmulatorPage{}
        }
    }
}

