/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

ListItem.Expandable {
    id: expandingColumnItem
    expandedHeight: contentColumn.height + units.gu(1)
    collapsedHeight: headerRow.childrenRect.height
    divider.visible: false

    property string title
    default property alias data: contentColumn.data

    onClicked: {
        expanded = !expanded;
    }

    Column {
        id: contentColumn
        anchors { left: parent.left; right: parent.right }
        Row{
            id: headerRow
            anchors { left: parent.left; right: parent.right}
            height: childrenRect.height
            Icon {
                name: expandingColumnItem.expanded ? "view-collapse" : "view-expand"
                color: UbuntuColors.orange
                width: 24
                height: 24
            }
            Item{
                width: units.gu(1)
                height: parent.height
            }
            Label {
                id: label
                text: expandingColumnItem.title
                fontSize: "large"
            }
        }
    }
}
