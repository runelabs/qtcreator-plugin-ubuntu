import QtQuick 2.0
import QtQuick.Layouts 1.0
import Ubuntu.Components 1.0
import Ubuntu.Components.ListItems 1.0 as ListItem
import Ubuntu.DevicesModel 0.1

ListItem.Standard {
    id: item
    property var input: null
    property string inputRole
    property alias checkable: switchbox.enabled

    onInputChanged: {
        if(input == FeatureState.Available)
            switchbox.checked = true;
        else
            switchbox.checked = false;
    }

    selected: false
    control: Row {
        ActivityIndicator {
            visible: input === FeatureState.Unknown
            running: visible
        }
        Switch {
            id: switchbox
            visible: input !== FeatureState.Unknown
            checked: input === FeatureState.Available
            enabled: checkable
            onCheckedChanged: {
                if (checked && input == FeatureState.NotAvailable) {
                    devicesModel.set(index,inputRole,true);
                    checked = true;
                }
                else if ((!checked) && input == FeatureState.Available) {
                    devicesModel.set(index,inputRole,false);
                    checked = false;
                }
            }
        }
    }
}
