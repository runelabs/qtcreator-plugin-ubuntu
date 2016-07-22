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
#ifndef UBUNTU_INTERNAL_UBUNTUABSTRACTGUIEDITOR_H
#define UBUNTU_INTERNAL_UBUNTUABSTRACTGUIEDITOR_H

#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/texteditor.h>

class QToolBar;
class QActionGroup;

namespace Ubuntu {
namespace Internal {

class UbuntuAbstractGuiEditorWidget;

class UbuntuAbstractGuiEditor : public Core::IEditor
{
    Q_OBJECT
public:
    UbuntuAbstractGuiEditor(const Core::Context &context);

    QWidget *toolBar() override;
    UbuntuAbstractGuiEditorWidget *editorWidget() const;
    Core::IDocument *document() override;
    TextEditor::TextEditorWidget *textEditor() const;

    int currentLine() const override;
    int currentColumn() const override;
    void gotoLine(int line, int column = 0, bool centerLine = true) override { textEditor()->gotoLine(line, column, centerLine); }

protected:
    virtual UbuntuAbstractGuiEditorWidget * createGuiEditor () = 0;
    void createUi ();

private:
    void syncCurrentAction ();

private slots:
    void changeEditorPage(QAction *action);

private:
    QString m_displayName;
    QToolBar *m_toolBar;
    QActionGroup *m_actionGroup;
};
} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUABSTRACTGUIEDITOR_H
