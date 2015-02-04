import QtQuick 2.0
import Ubuntu.Components 1.0
import Ubuntu.Components.Popups 1.0
import "../Components"

Dialog {
    id: dialogue
    title: deviceId >= 0 ? i18n.tr("Delete device") : i18n.tr("Delete emulator")
    text:  deviceId >= 0 ? i18n.tr("Are you sure you want to delete this device?") : i18n.tr("Are you sure you want to delete this emulator?")

    property string emulatorImageName
    property int    deviceId: -1

    Button {
        text: i18n.tr("Cancel")
        color: UbuntuColors.warmGrey
        onClicked: PopupUtils.close(dialogue)
    }
    Button {
        text: i18n.tr("Delete")
        color: UbuntuColors.orange
        onClicked: {
            if(deviceId >= 0) {
                console.log("Deleting device: "+dialogue.deviceId);
                devicesModel.deleteDevice(dialogue.deviceId);
            } else {
                console.log("Deleting emu: "+dialogue.emulatorImageName);
                devicesModel.deleteEmulator(dialogue.emulatorImageName);
            }
            PopupUtils.close(dialogue);
        }
    }
}
