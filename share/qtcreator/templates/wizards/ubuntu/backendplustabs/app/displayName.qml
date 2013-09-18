import QtQuick 2.0
import Ubuntu.Components 0.1
import "ui"

/*!
    \brief MainView with Tabs element.
           First Tab has a single Label and
           second Tab has a single ToolbarAction.
*/

MainView {
    // objectName for functional testing purposes (autopilot-qt5)
    objectName: "mainView"

    // Note! applicationName needs to match the "name" field of the click manifest
    applicationName: "com.ubuntu.developer.%BZR_USERNAME%.%DISPLAYNAME%"

    /* 
     This property enables the application to change orientation 
     when the device is rotated. The default is false.
    */
    //automaticOrientation: true

    width: units.gu(100)
    height: units.gu(75)

    Tabs {
        id: tabs

        HelloTab {
            objectName: "helloTab"
        }

        WorldTab {
            objectName: "worldTab"
        }
    }
}
