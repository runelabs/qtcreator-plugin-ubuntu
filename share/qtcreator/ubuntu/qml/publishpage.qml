import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3
import "Components"

MainView {
    width: units.gu(48)
    height: units.gu(60)

    Component {
        id: validationDelegate
        Rectangle {
            height: descText.height
            width: scrollView.width
            color: !(index % 2) ? "#FFFFFF" : "#E5E4E2"

            //block mouse events
            MouseArea {
                anchors.fill: parent
            }

            Row {
                id: headerRow
                height: descText.height
                spacing: 2

                //spacing
                Item{
                    width: units.gu(5)
                    height: 1
                }

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16
                    height: 16
                    source: ImageRole
                }

                Column {
                    id: descText
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: display + "\n" + DescriptionRole
                    }

                    Link {
                        width: parent.width
                        height: LinkRole.length > 0 ? 30 : 0
                        title: LinkRole
                        link: LinkRole
                    }
                }
            }
        }
    }

    Page {
        title: "Ubuntu Publish"

        Item {
            id: spacing
            anchors.top: parent.top
            height: units.gu(2)
            width: 1
        }

        Connections {
            target: publishModel
            onBeginValidation: {
                validationSection.expanded=true;
                logSection.expanded=false;
            }
        }

        Row {
            id: topRow
            anchors.top: spacing.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: units.gu(2)
            Button {
                visible: publishModel.showValidationUi
                text: "Validate existing click package"
                onClicked: {
                    publishModel.on_pushButtonReviewersTools_clicked();
                }
            }
            Button {
                visible: publishModel.showValidationUi
                enabled: publishModel.canBuild
                text: "Build and validate click package"
                onClicked: {
                    publishModel.on_pushButtonClickPackage_clicked();
                }
            }
            Button {
                text: "Install on device"
                onClicked: {
                    publishModel.log = "";
                    publishModel.buildAndInstallPackageRequested();
                    validationSection.expanded=false;
                    logSection.expanded=true;
                }
            }
        }

        Row {
            id: warningsRow
            anchors.top: topRow.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: units.gu(2)
            spacing: units.gu(2)
            visible: !publishModel.canBuild
            Icon {
                source: "qrc:/ubuntu/images/warning.png"
            }
            Label {
                text: "The selected Kit does not support building click packages. Please make sure to select a Ubuntu Kit."
            }
        }

        ScrollableView {
            id: scrollView
            clip: true

            anchors.top: warningsRow.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            SectionItem {
                id: validationSection
                title: "Validation"
                visible: publishModel.showValidationUi
                Repeater {
                    model: VisualDataModel{
                        model: publishModel.validationModel
                        delegate: SectionItem{
                            title: TypeRole
                            imageSource: ImageRole
                            expanded: ShouldExpandRole
                            Column {
                                Repeater{
                                    model: VisualDataModel{
                                        model: publishModel.validationModel
                                        rootIndex: modelIndex(index)
                                        delegate: validationDelegate
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SectionItem {
                id: logSection
                title: "Log"
                expanded: true
                TextArea {
                    autoSize: true
                    maximumLineCount: 0
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: units.gu(60)
                    highlighted: true

                    readOnly: true
                    text: publishModel.log
                    textFormat: TextEdit.AutoText
                }
            }
        }
    }
}
