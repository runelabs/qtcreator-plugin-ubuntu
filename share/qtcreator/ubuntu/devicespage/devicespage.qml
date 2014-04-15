import QtQuick 2.0
import QtQuick.Controls 1.0 as Controls
import QtQuick.Layouts 1.0
import Ubuntu.Components 0.1

MainView {
    Page {
        title: "Ubuntu Devices"
        flickable: null
        Controls.SplitView {
            orientation: Qt.Horizontal
            anchors.fill: parent

            Controls.ScrollView {
                width: 200
                Layout.fillHeight: true
                Layout.minimumWidth: 200
                Layout.maximumWidth: 400

                UbuntuListView {
                    id: devicesList
                    model: devicesModel
                    highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
                    delegate: Item {
                        id: delegate
                        width:parent.width
                        height: theTxt.height+2
                        RowLayout {
                            Text{
                                id: theTxt
                                text: display
                                clip: true
                                wrapMode: Text.WordWrap
                            }
                        }
                        MouseArea {
                            hoverEnabled: false
                            anchors.fill: parent
                            onClicked: devicesList.currentIndex=index
                        }
                    }
                }
            }

            Item {
                id: centerItem
                Layout.minimumWidth: 50
                Layout.fillWidth: true
                property int currentIndex: devicesList.currentIndex
                Repeater {
                    model: devicesModel
                    Item{
                        anchors.fill: parent
                        anchors.margins: 12
                        visible: index == devicesList.currentIndex

                        GridLayout {
                            columns: 2
                            Label {
                                text: i18n.tr("Current State")
                                Layout.alignment: Qt.AlignTop
                            }
                            Controls.Label {
                                text: connectionState
                            }
                            Label {
                                text: "Kits"
                                Layout.alignment: Qt.AlignTop
                            }
                            Rectangle {
                                color: "white"
                                Layout.maximumHeight: 200
                                Layout.minimumHeight: 100
                                width: 300
                                Controls.ScrollView {
                                    frameVisible: true
                                    anchors.fill: parent
                                    UbuntuListView {
                                        anchors.fill: parent
                                        flickableDirection: Flickable.HorizontalAndVerticalFlick
                                        model: kits
                                        delegate:
                                            Text {
                                            height:20
                                            text: {
                                                return modelData.displayName;
                                            }
                                        }
                                    }
                                }
                            }
                            FeatureStateItem {
                                Layout.columnSpan: 2
                                text: "Has devloper mode enabled"
                                input: developerModeEnabled
                                height: 24
                            }
                            FeatureStateItem {
                                Layout.columnSpan: 2
                                text: "Has network connection"
                                input: hasNetworkConnection
                                height: 24
                            }
                            FeatureStateItem {
                                Layout.columnSpan: 2
                                text: "Has writeable image"
                                input: hasWriteableImage
                                height: 24
                            }
                            FeatureStateItem {
                                Layout.columnSpan: 2
                                text: "Has device developer tools"
                                input: hasDeveloperTools
                                height: 24
                            }
                        }
                    }
                }
            }
        }
    }
}

