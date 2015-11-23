import QtQuick 2.4
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.Components.Popups 0.1

Item {
    Controls.TextArea {
        id: logTextArea
        anchors.fill: parent
        readOnly: true
        textFormat: TextEdit.AutoText
        Component.onCompleted: {
            deviceMode.appendText.connect(appendToLog);
        }
        function appendToLog (txt) {
            logTextArea.append(txt);
        }
    }
}
