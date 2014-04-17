import QtQuick 2.0

ListModel {
    id: devicesModel
    ListElement {
        display: "Ubuntu Device Nexus S"
        isEmulator: false
        connectionState: 1
        connectionStateString: "Connected"
        developerModeEnabled: 0
        hasNetworkConnection: 0
        hasWriteableImage: 1
        hasDeveloperTools: 2
        kits: [
            ListElement{
                kitId: "1"
                displayName: "Nexus S UbuntuSDK for armhf (GCC ubuntu-sdk-14.04-trusty)"
            }
        ]
        deviceLog: "<p><strong>Checking for network.....</strong><strong> </strong><br/>available</p><p><strong>Checking for readable Image....</strong><br/>not available</p>"
    }
    ListElement {
        display: "Ubuntu Emulator"
        isEmulator: true
        connectionState: 1
        connectionStateString: "Connected"
        developerModeEnabled: 1
        hasNetworkConnection: 1
        hasWriteableImage: 1
        hasDeveloperTools: 1
        kits: [
        ]
        deviceLog: "<p><strong>Checking for network.....</strong><strong> </strong><br/>available</p><p><strong>Checking for readable Image....</strong><br/>not available</p>"
    }
    ListElement {
        display: "Ubuntu Device Tablet"
        isEmulator: false
        connectionState: 0
        connectionStateString: "Disconnected"
        developerModeEnabled: 0
        hasNetworkConnection: 0
        hasWriteableImage: 0
        hasDeveloperTools: 0
        kits: [
            ListElement{
                kitId: "1"
                displayName: "Tablet UbuntuSDK for armhf (GCC ubuntu-sdk-14.04-trusty)"
            }
        ]
        deviceLog: "<p><strong>Checking for network.....</strong><strong> </strong><br/>available</p><p><strong>Checking for readable Image....</strong><br/>not available</p>"
    }
}
