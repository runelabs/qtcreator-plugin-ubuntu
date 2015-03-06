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

#include "ubuntuapparmoreditor.h"
#include "ubuntuconstants.h"
#include "ubuntuclickmanifest.h"
#include "ubuntusecuritypolicypickerdialog.h"

#include <texteditor/textdocument.h>

#include <QMenu>

namespace Ubuntu {
namespace Internal {

UbuntuApparmorEditor::UbuntuApparmorEditor()
    : UbuntuAbstractGuiEditor(Core::Context(Constants::UBUNTU_APPARMOR_EDITOR_CONTEXT)),
      m_editorWidget(0)
{
    createUi();
}

UbuntuApparmorEditor::~UbuntuApparmorEditor()
{
    if(m_editorWidget)
        delete m_editorWidget;
}

UbuntuApparmorEditorWidget *UbuntuApparmorEditor::guiEditor() const
{
    return m_editorWidget;
}

UbuntuAbstractGuiEditorWidget *UbuntuApparmorEditor::createGuiEditor()
{
    if(m_editorWidget == 0)
        m_editorWidget = new UbuntuApparmorEditorWidget();

    return m_editorWidget;
}

UbuntuApparmorEditorWidget::UbuntuApparmorEditorWidget() :
    UbuntuAbstractGuiEditorWidget(QLatin1String(Constants::UBUNTU_APPARMOR_MIME_TYPE)),
    m_ui(0)
{
    createUI();
}

UbuntuApparmorEditorWidget::~UbuntuApparmorEditorWidget()
{
    if(m_ui)
        delete m_ui;
}

bool UbuntuApparmorEditorWidget::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    bool result = UbuntuAbstractGuiEditorWidget::open(errorString,fileName,realFileName);

    if(!result)
        return result;

    //let see if we have valid data
    m_apparmor = QSharedPointer<UbuntuClickManifest>(new UbuntuClickManifest);
    if(m_apparmor->loadFromString(m_sourceEditor->toPlainText())) {
        if(activePage() != Source)
            syncToWidgets(m_apparmor.data());
        return true;
    } else {
        //switch to source page without syncing
        m_widgetStack->setCurrentIndex(Source);
        updateInfoBar(tr("There is a error in the file, please check the syntax."));
    }

    //ops something went wrong, we need to show the error somewhere
    return true;
}

void UbuntuApparmorEditorWidget::setVersion(const QString &version)
{
    //make sure all changes are in source
    if(activePage() != Source)
        syncToSource();

    UbuntuClickManifest aa;
    if(aa.loadFromString(textEditorWidget()->textDocument()->plainText())) {
        aa.setPolicyVersion(version);
        textEditorWidget()->textDocument()->setPlainText(aa.raw()+QStringLiteral("\n"));
        textEditorWidget()->document()->setModified(true);

        if(activePage() == General)
            syncToWidgets();
    }
}

bool UbuntuApparmorEditorWidget::syncToWidgets()
{
    QSharedPointer<UbuntuClickManifest> man(new UbuntuClickManifest);
    if(man->loadFromString(m_sourceEditor->toPlainText())) {
        m_apparmor.swap(man);
        syncToWidgets(m_apparmor.data());
        updateInfoBar(QString());
        return true;
    }

    QString text = tr("There is a error in the file, please check the syntax.");
    updateInfoBar(text);
    return false;
}

bool UbuntuApparmorEditorWidget::syncToWidgets(UbuntuClickManifest *source)
{
    QStringList groups = source->policyGroups();

    QStringList currGroups;
    for (int i=0; i < m_ui->listWidget->count(); i++) {
        // Fix bug #1221407 - make sure that there are no empty policy groups.
        QString policyGroup = m_ui->listWidget->item(i)->text().trimmed();
        if (!policyGroup.isEmpty()) {
            currGroups.append(m_ui->listWidget->item(i)->text());
        }
    }

    if(groups != currGroups){
        m_ui->listWidget->clear();
        foreach(const QString &polGroup, groups) {
            m_ui->listWidget->addItem(polGroup);
        }
    }

    m_dirty = false;
    emit uiEditorChanged();
    return true;
}

void UbuntuApparmorEditorWidget::syncToSource()
{
    QStringList items;
    for (int i=0; i < m_ui->listWidget->count(); i++) {
        // Fix bug #1221407 - make sure that there are no empty policy groups.
        QString policyGroup = m_ui->listWidget->item(i)->text().trimmed();
        if (!policyGroup.isEmpty()) {
            items.append(m_ui->listWidget->item(i)->text());
        }
    }

    m_apparmor->setPolicyGroups(items);

    QString result = m_apparmor->raw()+QStringLiteral("\n");
    QString src    = m_sourceEditor->toPlainText();
    if (result == src)
        return;

    m_sourceEditor->setPlainText(result);
    m_sourceEditor->document()->setModified(true);

    m_dirty = false;
    emit uiEditorChanged();
}

QWidget *UbuntuApparmorEditorWidget::createMainWidget()
{
    QWidget *mainWidget = new QWidget;
    m_ui = new Ui::UbuntuAppArmorEditor();
    m_ui->setupUi(mainWidget);

    m_ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_ui->listWidget,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(setDirty()));
    connect(m_ui->listWidget,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(on_listWidget_customContextMenuRequested(QPoint)));
    connect(m_ui->listWidget->model(),SIGNAL(rowsInserted(QModelIndex,int,int)),this,SLOT(setDirty()));
    connect(m_ui->listWidget->model(),SIGNAL(columnsInserted(QModelIndex,int,int)),this,SLOT(setDirty()));
    connect(m_ui->listWidget->model(),SIGNAL(rowsRemoved(QModelIndex,int,int)),this,SLOT(setDirty()));
    connect(m_ui->listWidget->model(),SIGNAL(columnsRemoved(QModelIndex,int,int)),this,SLOT(setDirty()));
    connect(m_ui->pushButton_addpolicy,SIGNAL(clicked()),this,SLOT(on_pushButton_addpolicy_clicked()));

    return mainWidget;
}

void UbuntuApparmorEditorWidget::on_pushButton_addpolicy_clicked()
{
    UbuntuSecurityPolicyPickerDialog dialog(m_apparmor->policyVersion());
    if (dialog.exec()) {
        m_ui->listWidget->addItems(dialog.selectedPolicyGroups());
    }
}

void UbuntuApparmorEditorWidget::on_listWidget_customContextMenuRequested(const QPoint &p) {
    if (m_ui->listWidget->selectedItems().count()==0) { return; }

    QMenu contextMenu;
    contextMenu.addAction(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_MENU_REMOVE));
    QAction* selectedItem = contextMenu.exec(m_ui->listWidget->mapToGlobal(p));
    if (selectedItem) {
        delete m_ui->listWidget->currentItem();
    }
}


} // namespace Internal
} // namespace Ubuntu
