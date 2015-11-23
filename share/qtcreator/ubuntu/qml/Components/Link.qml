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

import QtQuick 2.4

Rectangle {
    id: linkRoot
    width: linkTitle.paintedWidth
    height: 60
    color: "transparent"
    opacity: 0.8

    property alias title : linkTitle.text
    property alias pixelSize : linkTitle.font.pixelSize
    property url link
    property color defaultColor : "#DD4814"
    property color hoverColor : "#eee"

    signal clicked()

    Rectangle {
        id: background
        color: hoverColor
        opacity: 0
        anchors.fill: parent

        Behavior on opacity {
            NumberAnimation { duration: 250 }
        }
    }

    Text {
        id: linkTitle
        anchors.centerIn: parent
        wrapMode: Text.WordWrap
        width: parent.width - 20
        font.family: "Ubuntu"
        font.letterSpacing: 1.5
        color: defaultColor
        font.weight: Font.Light
        textFormat: Text.RichText
        text: ""
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onEntered: {
            background.opacity = 1;
        }
        onExited: {
            background.opacity = 0;
        }

        onClicked: {
            if (link !== undefined) Qt.openUrlExternally(link);

            linkRoot.clicked()
        }
        onPressed: {
            linkTitle.color = Qt.lighter(defaultColor);
        }
        onReleased: {
            linkTitle.color = defaultColor;
        }
    }

}
