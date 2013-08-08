import QtQuick 2.0
import Ubuntu.Components 0.1
import "../components"

Tab {
    title: i18n.tr("Hello..")

    page: Page {
        Column {
            spacing: units.gu(2)
            anchors.centerIn: parent

            HelloComponent {
                objectName: "helloTab_HelloComponent"

                anchors.horizontalCenter: parent.horizontalCenter

                text: i18n.tr("HelloTab")
            }

            Label {
                objectName: "helloTab_label"

                anchors.horizontalCenter: parent.horizontalCenter

                text: i18n.tr("You can change the Tab from Page title above.")
            }
        }
    }
}
