import QtQuick 2.0
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

ListItem.Standard {
    id: item
    property var input: null
    selected: false
    control: Row {
        ActivityIndicator {
            visible: input === 1
            running: true
        }
        Switch {
            visible: input !== 1
            checked: input === 2
        }
    }
}
