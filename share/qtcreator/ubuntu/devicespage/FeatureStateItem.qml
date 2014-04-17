import QtQuick 2.0
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.DevicesModel 0.1

ListItem.Standard {
    id: item
    property var input: null
    property string inputRole
    property alias checkable: switchbox.enabled
    selected: false
    control: Row {
        ActivityIndicator {
            visible: input === States.Detecting
            running: true
        }
        Switch {
            id: switchbox
            visible: input !== States.Detecting
            checked: input === States.Available
            enabled: checkable
            onCheckedChanged: {
                if(input == States.Detecting)
                    return;

                console.log(checked+" "+input);

                if (checked && input == States.NotAvailable) {
                    if(!devicesModel.set(index,inputRole,true))
                        checked = false;
                }
                else if ((!checked) && input == States.Available) {
                    if(!devicesModel.set(index,inputRole,false))
                        checked = true;
                }
            }
        }
    }
}
