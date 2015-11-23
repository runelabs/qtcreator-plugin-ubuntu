import QtQuick 2.4
import Ubuntu.Components 1.3
import %ClickHookName:s% 1.0

/*!
    \brief MainView with Tabs element.
           First Tab has a single Label and
           second Tab has a single ToolbarAction.
*/

MainView {
    // objectName for functional testing purposes (autopilot-qt5)
    objectName: "mainView"

    // Note! applicationName needs to match the "name" field of the click manifest
    applicationName: "%ProjectName:l%.%ClickDomain:l%"

    /* 
     This property enables the application to change orientation 
     when the device is rotated. The default is false.
    */
    //automaticOrientation: true


    width: units.gu(100)
    height: units.gu(76)

    Page {
         title: i18n.tr("%ClickHookName%")

         MyType {
             id: myType

             Component.onCompleted: {
                 myType.helloWorld = i18n.tr("Hello world..")
             }
         }

         Column {
             spacing: units.gu(1)
             anchors {
                 margins: units.gu(2)
                 fill: parent
             }

             Label {
                 id: label
                 objectName: "label"

                 text: myType.helloWorld
             }

             Button {
                 objectName: "button"
                 width: parent.width

                 text: i18n.tr("Tap me!")

                 onClicked: {
                     myType.helloWorld = i18n.tr("..from Cpp Backend")
                 }
             }
         }
     }
}
