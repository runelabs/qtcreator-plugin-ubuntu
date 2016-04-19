/*
 * Copyright 2016 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import "Components"


Rectangle {
    color: "#F7F6F5"

    property int maximumWidth : 900

    anchors.fill: parent


    Image {
        anchors.fill: parent
        fillMode: Image.Tile
        source: "images/bg_dotted.png"
    }

    Rectangle {
        id: contentBackground
        color: "#fff"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 0
        width: parent.width > maximumWidth ? maximumWidth : parent.width
    }

    ScrollView {
        anchors.fill: parent

        ColumnLayout {
            id: topLayout
            x: contentBackground.x
            width: contentBackground.width
            spacing: 0


            GridLayout {
                id: grid
                Layout.alignment: Qt.AlignTop
                Layout.leftMargin: 20
                Layout.rightMargin: 0
                Layout.topMargin: 10

                Layout.fillWidth: true
                Layout.maximumWidth: topLayout.width - 20

                flow: GridLayout.TopToBottom
                rows: 3

                Text {
                    wrapMode: Text.WordWrap
                    font.family: "Ubuntu"
                    font.weight: Font.Light
                    font.pointSize: 42
                    font.letterSpacing: 2
                    text: "Make it Ubuntu"

                    Layout.fillWidth: true
                    Layout.topMargin: 30
                }

                Text {
                    wrapMode: Text.WordWrap
                    font.family: "Ubuntu"
                    font.weight: Font.Light
                    font.pointSize: 20
                    font.letterSpacing: 1.5
                    textFormat: Text.RichText
                    text: "<style> a { text-decoration: none; color: #DD4814; cursor: pointer } </style><a href=\"http://developer.ubuntu.com/get-started/\">Get started</a> today with the Ubuntu SDK and the App Design Guides."
                    onLinkActivated: {
                        Qt.openUrlExternally(link);
                    }
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                Column {
                    Layout.rowSpan: 3
                    Layout.margins: 0
                    Layout.minimumWidth: topLayout.width / 2
                    Layout.maximumWidth: topLayout.width / 2
                    Layout.alignment: Qt.AlignTop
                    Image {
                        width: parent.width
                        source: "images/devices.png"
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }

            Column {
                id: topLinks

                Layout.minimumWidth: topLayout.width
                Layout.maximumWidth: topLayout.width
                Layout.margins: 20
                Layout.topMargin: 0

                spacing: 5

                Link {
                    width: parent.width
                    title: "Create a New Project &gt;"
                    onClicked: ubuntuWelcomeMode.newProject()
                    pixelSize: 20
                }

                Link {
                    width: parent.width
                    title: "See Ubuntu Touch core apps @ Launchpad &gt;"
                    link: "https://launchpad.net/ubuntu-phone-coreapps/"
                    pixelSize: 20
                }

                Link {
                    width: parent.width
                    title: "Design something beautiful &gt;"
                    link: "http://design.ubuntu.com/apps"
                    pixelSize: 20
                }

                Link {
                    width: parent.width
                    title: "Build something solid in QML &gt;"
                    link: "http://developer.ubuntu.com/api/qml/current/"
                    pixelSize: 20
                }
                Link {
                    width: parent.width
                    title: "Build something solid in HTML5 &gt;"
                    link: "http://developer.ubuntu.com/api/html5/current/"
                    pixelSize: 20
                }
                Link {
                    width: parent.width
                    title: "Open UI Toolkit component gallery &gt;"
                    onClicked: ubuntuWelcomeMode.openGallery()
                    pixelSize: 20
                }
                Link {
                    width: parent.width
                    title: "Stay up to date with the developer community &gt;"
                    link: "http://developer.ubuntu.com/blog"
                    pixelSize: 20
                }
            }

            Column {
                id: bottomBox

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
                Layout.topMargin: 20
                Layout.leftMargin: 20
                Layout.bottomMargin: 0

                spacing: 5
                Text {
                    wrapMode: Text.WordWrap
                    width: parent.width
                    font.family: "Ubuntu"
                    font.pixelSize: 26
                    font.weight: Font.Light
                    text: "Get in touch"
                }

                Text {
                    wrapMode: Text.WordWrap
                    width: parent.width
                    font.family: "Ubuntu"
                    font.pixelSize: 18
                    textFormat: Text.RichText
                    font.weight: Font.Light
                    text: "<style> a { text-decoration: none; color: #DD4814; cursor: pointer } </style>By joining our <a href=\"http://wiki.ubuntu.com/Touch/Contribute#Discuss\">mailing list</a> and for real-time communication join <a href=\"http://webchat.freenode.net/?channels=ubuntu-touch\">#ubuntu-touch</a> on freenode."
                    onLinkActivated: {
                        Qt.openUrlExternally(link);
                    }
                }
            }
        }
    }
}
