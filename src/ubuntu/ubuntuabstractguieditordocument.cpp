/*
 * Copyright 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "ubuntuabstractguieditordocument.h"
#include "ubuntuabstractguieditor.h"
#include "ubuntuabstractguieditorwidget.h"
#include "ubuntuconstants.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QDir>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuAbstractGuiEditorDocument::UbuntuAbstractGuiEditorDocument
 *  This is basically a bare PlainTextDocument, however in order to be
 *  able to sync between the Widget based and Text based manifest editor
 *  we need to hook into save() and isModified()
 */
UbuntuAbstractGuiEditorDocument::UbuntuAbstractGuiEditorDocument(const QString &mimeType, UbuntuAbstractGuiEditorWidget *editorWidget)
    : TextEditor::TextDocument(),
      m_editorWidget(editorWidget)
{
    setMimeType(mimeType);
    connect(editorWidget, SIGNAL(uiEditorChanged()),this,SIGNAL(changed()));
}

bool UbuntuAbstractGuiEditorDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    if(!m_editorWidget->preSave()) {
        *errorString = tr("Please check the info box in the editor.");
        return false;
    }

    if(TextDocument::save(errorString, fileName, autoSave)) {
        m_editorWidget->saved();
        return true;
    }
    return false;
}

QString UbuntuAbstractGuiEditorDocument::defaultPath() const
{
    return filePath().toFileInfo().absolutePath();
}

QString UbuntuAbstractGuiEditorDocument::suggestedFileName() const
{
    return filePath().toFileInfo().fileName();
}

bool UbuntuAbstractGuiEditorDocument::isModified() const
{
    return TextDocument::isModified() ||  m_editorWidget->isModified();
}

bool UbuntuAbstractGuiEditorDocument::isSaveAsAllowed() const
{
    return false;
}

} // namespace Internal
} // namespace Ubuntu
