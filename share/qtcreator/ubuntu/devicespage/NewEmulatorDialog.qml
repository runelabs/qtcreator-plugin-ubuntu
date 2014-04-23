import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.Popups 0.1

Dialog {
    title: i18n.tr("Create emulator")
    text:  i18n.tr("Please select a name for the emulator")
    modal: true
    TextField {
        id: inputName
        placeholderText: i18n.tr("Emulator name")
        property string lastError
        onTextChanged: {
            var result = emulatorModel.validateEmulatorName(text);
            acceptableInput = result.valid;
            lastError       = result.error;
        }
    }
    Button {
        text: "cancel"
        onClicked: PopupUtils.close(dialogue)
    }
    Button {
        text: "create"
        color: UbuntuColors.orange
        onClicked: PopupUtils.close(dialogue)
    }
}

