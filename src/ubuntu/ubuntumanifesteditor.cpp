/*
 * Copyright 2014 Canonical Ltd.
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
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#include "ubuntumanifesteditor.h"
#include "ubuntumanifesteditorwidget.h"
#include "ubuntuconstants.h"

#include <texteditor/texteditorconstants.h>

#include <QActionGroup>
#include <QToolBar>
#include <QTextBlock>

namespace Ubuntu {
namespace Internal {

UbuntuManifestEditor::UbuntuManifestEditor()
    : UbuntuAbstractGuiEditor(Core::Context(Constants::UBUNTU_MANIFEST_EDITOR_CONTEXT)),
      m_editorWidget(0)
{
    createUi();
}

UbuntuManifestEditor::~UbuntuManifestEditor()
{
    if(m_editorWidget)
        delete m_editorWidget;
}

UbuntuAbstractGuiEditorWidget *UbuntuManifestEditor::createGuiEditor()
{
    if(m_editorWidget == 0)
        m_editorWidget = new UbuntuManifestEditorWidget();

    return m_editorWidget;
}

} // namespace Internal
} // namespace Ubuntu
