import QtQuick 2.4
import QtQuick.Controls 1.3 as Controls
import QtQuick.Controls.Styles 1.3
import Ubuntu.Components 1.3

Controls.ToolBar {
    id: toolbar

    style: ToolBarStyle {
        background: Rectangle {
            id: styledItem
            implicitWidth: 100
            implicitHeight: units.gu(5) + divider.height
            color: theme.palette.normal.background

            Rectangle {
                id: divider
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: units.dp(1)
                color: Qt.darker(theme.palette.normal.background, 1.1)
            }
        }
    }
}
