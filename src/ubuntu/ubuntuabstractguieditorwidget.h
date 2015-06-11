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

#ifndef UBUNTUABSTRACTGUIEDITORWIDGET_H
#define UBUNTUABSTRACTGUIEDITORWIDGET_H

#include "ubuntuclickmanifest.h"

#include <texteditor/texteditor.h>

#include <QScrollArea>
#include "ui_ubuntumanifesteditor.h"

class QStackedWidget;

namespace ProjectExplorer {class Project;}

namespace Ubuntu {
namespace Internal {

class UbuntuAbstractGuiEditor;
class UbuntuAbstractGuiEditorWidget;
class UbuntuClickManifest;

class UbuntuManifestTextEditorWidget : public TextEditor::TextEditorWidget
{
public:
    UbuntuManifestTextEditorWidget(QString mimeType, UbuntuAbstractGuiEditorWidget *parent = 0);
protected:
    UbuntuAbstractGuiEditorWidget *m_parent;
    QString m_mimeType;
};

class UbuntuAbstractGuiEditorWidget : public QScrollArea
{
    Q_OBJECT
public:
    enum EditorPage {
        General = 0,
        Source = 1
    };

    explicit UbuntuAbstractGuiEditorWidget(const QString &mimeType);
    ~UbuntuAbstractGuiEditorWidget();

    virtual bool isModified() const;

    EditorPage activePage() const;
    bool setActivePage(EditorPage page);

    bool preSave();

    TextEditor::TextEditorWidget *textEditorWidget() const;

    virtual void saved ();

protected slots:
    void setDirty ();
    void createUI ();

protected:
    virtual void aboutToOpen(const QString &, const QString &);
    virtual void updateAfterFileLoad ();
    virtual bool syncToWidgets () = 0;
    virtual void syncToSource  () = 0;
    virtual QWidget *createMainWidget () = 0;

    void updateInfoBar(const QString &errorMessage);

signals:
    void uiEditorChanged();
    void editorViewChanged();

protected:
    QStackedWidget *m_widgetStack;
    TextEditor::TextEditorWidget *m_sourceEditor;

    bool m_dirty;
};

ProjectExplorer::Project *ubuntuProject(const QString &file);

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTUABSTRACTGUIEDITORWIDGET_H
