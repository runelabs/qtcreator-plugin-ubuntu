import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.Popups 0.1

Dialog {
    id: dialogue
    title: i18n.tr("Create emulator")
    text:  i18n.tr("Please select a name for the emulator")
    modal: true
    TextField {
        id: inputName
        placeholderText: i18n.tr("Emulator name")
        property string lastError
        property bool hasError
        onTextChanged: validate()
        Component.onCompleted: validate()
        function validate() {
            var result = emulatorModel.validateEmulatorName(text);
            hasError   = !result.valid;
            lastError  = result.error;
        }
    }
    Label {
        horizontalAlignment: Text.AlignHCenter
        text: inputName.lastError
        color: "red"
        visible: inputName.hasError
    }

    Button {
        text: "cancel"
        onClicked: PopupUtils.close(dialogue)
    }
    Button {
        text: "create"
        color: UbuntuColors.orange
        enabled: !inputName.hasError
        onClicked: {
            if(inputName.hasError)
                return;
            emulatorModel.createEmulatorImage(inputName.text);
            PopupUtils.close(dialogue);
        }
    }
}

