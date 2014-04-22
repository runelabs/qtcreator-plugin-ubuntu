import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem


Controls.TextArea {
    readOnly: true
    text: deviceLog
    textFormat: TextEdit.AutoText
}
