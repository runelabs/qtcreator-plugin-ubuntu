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

#ifndef UBUNTU_INTERNAL_UBUNTUAPPARMOREDITOR_H
#define UBUNTU_INTERNAL_UBUNTUAPPARMOREDITOR_H

#include "ubuntuabstractguieditor.h"
#include "ubuntuabstractguieditorwidget.h"
#include "ui_ubuntuapparmoreditor.h"

#include <QSharedPointer>

namespace Ubuntu {
namespace Internal {

class UbuntuClickManifest;
class UbuntuApparmorEditorWidget;

class UbuntuApparmorEditor : public UbuntuAbstractGuiEditor
{
    Q_OBJECT
public:
    UbuntuApparmorEditor();
    ~UbuntuApparmorEditor();

    UbuntuApparmorEditorWidget *guiEditor() const;

protected:
    // UbuntuAbstractGuiEditor interface
    virtual UbuntuAbstractGuiEditorWidget *createGuiEditor();

private:
    UbuntuApparmorEditorWidget *m_editorWidget;
};

class UbuntuApparmorEditorWidget : public UbuntuAbstractGuiEditorWidget
{
    Q_OBJECT
public:
    UbuntuApparmorEditorWidget();
    ~UbuntuApparmorEditorWidget();

    // UbuntuAbstractGuiEditorWidget interface
public:
    void setVersion (const QString &version);

protected:
    virtual void updateAfterFileLoad() override;
    virtual bool syncToWidgets() override;
    bool syncToWidgets(UbuntuClickManifest *source);
    virtual void syncToSource() override;
    virtual QWidget *createMainWidget() override;

protected slots:
    void on_pushButton_addpolicy_clicked();    
    void on_listWidget_customContextMenuRequested(const QPoint &p);

private:
    QSharedPointer<UbuntuClickManifest> m_apparmor;
    Ui::UbuntuAppArmorEditor *m_ui;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUAPPARMOREDITOR_H
