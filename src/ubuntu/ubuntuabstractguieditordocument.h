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

#ifndef UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H
#define UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

namespace Ubuntu {
namespace Internal {

class UbuntuAbstractGuiEditorWidget;

class UbuntuAbstractGuiEditorDocument : public TextEditor::TextDocument
{
public:
    UbuntuAbstractGuiEditorDocument(const QString &mimeType, UbuntuAbstractGuiEditorWidget *editorWidget);
    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);

    QString defaultPath() const override;
    QString suggestedFileName() const override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;
private:
    UbuntuAbstractGuiEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUMANIFESTDOCUMENT_H
