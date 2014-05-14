import QtQuick 2.0
import Ubuntu.Components 1.0
import Ubuntu.Components.Popups 1.0

Dialog {
    id: dialogue
    title: i18n.tr("Delete emulator")
    text: i18n.tr("Are you sure you want to delete this emulator?")
    Button {
        text: i18n.tr("cancel")
        onClicked: PopupUtils.close(dialogue)
    }
    Button {
        text: i18n.tr("delete")
        color: UbuntuColors.orange
        onClicked: {
            devicesModel.deleteEmulator(dialogue.emulatorImageName)
            PopupUtils.close(dialogue);
        }
    }
}
