import QtQuick 2.0
import Ubuntu.Components 1.0
import Ubuntu.Components.ListItems 1.0 as ListItem
import Ubuntu.Components.Popups 1.0

import "../Components"

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
            var result = devicesModel.validateEmulatorName(text);
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

    ListItem.ItemSelector {
        id: arch
        model: [i18n.tr("i386"),
            i18n.tr("armhf")]
    }

    ListItem.ItemSelector {
        id: channel
        model: ["devel",
                "devel-proposed",
                "stable",
                "rtm-14.09",
                "rtm-14.09-proposed",
                "custom channel"]
    }

    TextField {
        id: inputChannelName
        placeholderText: i18n.tr("Emulator channel")
        visible: channel.model[channel.selectedIndex] === "custom channel"
    }

    ListItem.ItemSelector {
        id: custom_pwd
        model: ["Use default password",
                "Set custom password"]
    }

    TextField {
        id: inputCustomPassword
        echoMode: TextInput.Password
        placeholderText: i18n.tr("Password")
        visible: custom_pwd.model[custom_pwd.selectedIndex] === "Set custom password"
    }


    Label {
        id: inputChannelNameError
        horizontalAlignment: Text.AlignHCenter
        text: "Channel name can not be empty"
        color: "red"
        visible: inputChannelName.visible && inputChannelName.text.length == 0
    }

    Button {
        text: "Cancel"
        color: UbuntuColors.warmGrey
        onClicked: PopupUtils.close(dialogue)
    }
    Button {
        text: "Create"
        color: UbuntuColors.orange
        enabled: !inputName.hasError && !inputChannelNameError.visible
        onClicked: {
            if(inputName.hasError || inputChannelNameError.visible)
                return;
            devicesModel.createEmulatorImage(inputName.text,
                                             arch.model[arch.selectedIndex],
                                             (inputChannelName.visible ? inputChannelName.text : channel.model[channel.selectedIndex]),
                                             (inputCustomPassword.visible ? inputCustomPassword.text : "0000")
                                             );
            PopupUtils.close(dialogue);
        }
    }
}

