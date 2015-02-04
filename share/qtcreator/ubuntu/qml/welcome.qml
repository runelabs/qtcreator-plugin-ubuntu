/*
 * Copyright 2013 Canonical Ltd.
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
 */

import QtQuick 2.0
import "Components"

Rectangle {
    color: "#F7F6F5"

    property int maximumWidth : 900
    property int maximumHeight: 579

    width: parent.width
    height: scrollView.height


    Image {
        anchors.fill: parent
        fillMode: Image.Tile
        source: "images/bg_dotted.png"
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        height: maximumHeight
        width: parent.width>maximumWidth ? maximumWidth : parent.width
        color: "#fff"

        Image {
            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.right: parent.right
            source: "images/devices.png"
            width: parent.width/2
            fillMode: Image.PreserveAspectFit
        }

        Rectangle {
            color: "transparent"
            anchors.margins: 20
            anchors.top: parent.top
            anchors.topMargin: 40
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - anchors.margins*2

            Column {
                id: headerColumn
                spacing: 20
                anchors.top: parent.top
                anchors.left: parent.left
                width: parent.width/2

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    font.family: "Ubuntu"
                    font.weight: Font.Light
                    font.pointSize: 42
                    font.letterSpacing: 2
                    text: "Make it Ubuntu"
                }

                Text {
                    wrapMode: Text.WordWrap
                    width: parent.width
                    font.family: "Ubuntu"
                    font.weight: Font.Light
                    font.pointSize: 20
                    font.letterSpacing: 1.5
                    textFormat: Text.RichText
                    text: "<style> a { text-decoration: none; color: #DD4814; cursor: pointer } </style><a href=\"http://developer.ubuntu.com/get-started/\">Get started</a> today with the Ubuntu SDK Preview and the App Design Guides."
                    onLinkActivated: {
                        Qt.openUrlExternally(link);
                    }

                }
            }

            Column {
                id: topLinks
                anchors.topMargin: 50
                anchors.top: headerColumn.bottom
                width: parent.width
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
            }
            Column {
                id: bottomBox
                anchors.top: topLinks.bottom
                anchors.left: parent.left
                anchors.topMargin: 20

                width: parent.width
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
            Row {
                anchors.topMargin: 20
                anchors.top: bottomBox.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: parent.width

                NewsBox {
                    height: parent.height
                    width: parent.width/2

                    link: "http://developer.ubuntu.com/feed/"
                }

                NewsBox {
                    height: parent.height
                    width: parent.width/2

                    link: "http://developer.ubuntu.com/category/event/feed/"
                }

            }
        }
    }
}

