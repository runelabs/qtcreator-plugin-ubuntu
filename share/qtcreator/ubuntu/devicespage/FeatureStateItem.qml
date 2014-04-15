import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1

Item {
    id: item
    property string text
    property var input: null
    RowLayout {
        Label {
            text: item.text
            Layout.fillWidth: true
        }
        ActivityIndicator {
            height: parent.height
            width: parent.height
            visible: true
            running: true
        }
    }

}
