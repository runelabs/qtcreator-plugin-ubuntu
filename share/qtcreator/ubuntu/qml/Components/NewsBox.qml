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
import QtQuick.XmlListModel 2.0

ListView {

    clip: true

    property url link

    model: XmlListModel {
        source: link
        query: "/rss/channel/item"

        XmlRole { name: "title"; query: "title/string()" }
        XmlRole { name: "link"; query: "link/string()" }
        XmlRole { name: "description"; query: "description/string()" }
        XmlRole { name: "pubDate"; query: "pubDate/string()" }
    }
    delegate: Item {
        id: delegate
        height: itemContent.height+ 20
        width: delegate.ListView.view.width
        Column {
            id: itemContent
            width: parent.width
            Text {
                text: pubDate
                font.pixelSize: 10
                font.family: "Ubuntu"
                font.weight: Font.Light
            }

            Text {
                text: title
                font.family: "Ubuntu"
                font.pixelSize: 20
                font.letterSpacing: 1.5
                wrapMode: Text.WordWrap
                font.weight: Font.Light
                width: parent.width
                baseUrl: link
            }

            Text {
                text: description
                font.family: "Ubuntu"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                font.weight: Font.Light
                font.letterSpacing: 1.5
                width: parent.width
                textFormat: Text.RichText
                baseUrl: link
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                Qt.openUrlExternally(link)
            }
        }
    }

}
