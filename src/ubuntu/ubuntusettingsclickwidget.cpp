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

#include "ubuntusettingsclickwidget.h"
#include "ui_ubuntusettingsclickwidget.h"
#include "ubuntuconstants.h"
#include "ubuntuclicktool.h"
#include "ubuntuclickdialog.h"
#include "settings.h"

#include <QFileDialog>
#include <QDir>
#include <QRegExp>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QDebug>

enum {
    debug = 0
};

namespace Ubuntu { namespace Internal {

UbuntuSettingsClickWidget::UbuntuSettingsClickWidget(QWidget *parent) :
    QWidget(parent),
    ui(new ::Ui::UbuntuSettingsClickWidget)
{
    ui->setupUi(this);

    //hide this setting for the moment, as we have no local mirror for lxd images
    ui->checkBoxLocalMirror->setVisible(false);

    Settings::ChrootSettings def = Settings::chrootSettings();

    ui->enableUpdateCheckerCheckBox->setChecked(def.autoCheckForUpdates);
    ui->checkBoxLocalMirror->setChecked(def.useLocalMirror);

    m_deleteMapper = new QSignalMapper(this);
    connect(m_deleteMapper, SIGNAL(mapped(int)),this, SLOT(on_deleteClickChroot(int)));
    m_maintainMapper = new QSignalMapper(this);
    connect(m_maintainMapper, SIGNAL(mapped(int)),this, SLOT(on_maintainClickChroot(int)));
    m_updateMapper = new QSignalMapper(this);
    connect(m_updateMapper, SIGNAL(mapped(int)),this, SLOT(on_upgradeClickChroot(int)));
    m_toggleUpgradeMapper = new QSignalMapper(this);
    connect(m_toggleUpgradeMapper, SIGNAL(mapped(int)),this, SLOT(on_toggleTargetUpgradeEnabled(int)));

    QStringList headers;
    headers << tr("Series")<< tr("Framework") << tr("Architecture")<< tr("Autoupgrade") << QLatin1String("")<<QLatin1String("")<<QLatin1String("");
    ui->treeWidgetClickTargets->setHeaderLabels(headers);
    ui->treeWidgetClickTargets->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->treeWidgetClickTargets->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->treeWidgetClickTargets->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeWidgetClickTargets->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->treeWidgetClickTargets->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->treeWidgetClickTargets->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->treeWidgetClickTargets->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    listExistingClickTargets();
}

void UbuntuSettingsClickWidget::apply() {

    Settings::ChrootSettings set;
    set.autoCheckForUpdates = ui->enableUpdateCheckerCheckBox->checkState() == Qt::Checked;
    set.useLocalMirror = ui->checkBoxLocalMirror->checkState() == Qt::Checked;
    Settings::setChrootSettings(set);
    Settings::flushSettings();
}

UbuntuSettingsClickWidget::~UbuntuSettingsClickWidget()
{
    delete ui;
}

void UbuntuSettingsClickWidget::on_pushButtonCreateClickTarget_clicked()
{
    //make sure the current settings are stored
    apply();

    Internal::UbuntuClickDialog::createClickChrootModal(true, this);
    listExistingClickTargets();
}

void UbuntuSettingsClickWidget::on_deleteClickChroot(const int index)
{
    if(index < 0 || index > m_availableTargets.size())
        return;

    if(debug) qDebug()<<"Destroying target "<< m_availableTargets.at(index);

    Internal::UbuntuClickDialog::maintainClickModal(m_availableTargets.at(index),UbuntuClickTool::Delete);
    listExistingClickTargets();
}

void UbuntuSettingsClickWidget::on_maintainClickChroot(const int index)
{
    if(index < 0 || index > m_availableTargets.size())
        return;
    UbuntuClickTool::openChrootTerminal(m_availableTargets.at(index));
}

void UbuntuSettingsClickWidget::on_upgradeClickChroot(const int index)
{
    if(index < 0 || index > m_availableTargets.size())
        return;
    Internal::UbuntuClickDialog::maintainClickModal(m_availableTargets.at(index),UbuntuClickTool::Upgrade);
}

void UbuntuSettingsClickWidget::on_toggleTargetUpgradeEnabled(const int index)
{
    if(index < 0 || index > m_availableTargets.size())
        return;

    QCheckBox *c = qobject_cast<QCheckBox *>(m_toggleUpgradeMapper->mapping(index));
    if(!c)
        return;

    UbuntuClickTool::setTargetUpgradesEnabled(m_availableTargets.at(index), c->checkState()==Qt::Checked);
    listExistingClickTargets();
}

/**
 * @brief UbuntuSettingsClickWidget::listExistingClickTargets
 * Lists all existing click targets in /var/lib/schroot/chroots
 * that match the click-<framework>-<arch> pattern
 */
void UbuntuSettingsClickWidget::listExistingClickTargets()
{
    //this should hopefully also delete all mapped pushbuttons
    ui->treeWidgetClickTargets->clear();

    QList<UbuntuClickTool::Target> items = UbuntuClickTool::listAvailableTargets();
    m_availableTargets = items;

    QAbstractItemModel* model = ui->treeWidgetClickTargets->model();

    //fill the treeview with all existing chroots
    for(int i = 0; i < items.size(); i++) {
        const UbuntuClickTool::Target& target = items.at(i);

        QTreeWidgetItem* chrootItem = new QTreeWidgetItem;
        chrootItem->setText(0,target.containerName);
        chrootItem->setText(1,target.framework);
        chrootItem->setText(2,target.architecture);
        ui->treeWidgetClickTargets->addTopLevelItem(chrootItem);

        QCheckBox* box = new QCheckBox();
        m_toggleUpgradeMapper->setMapping(box,i);
        box->setChecked(target.upgradesEnabled);
        connect(box, SIGNAL(stateChanged(int)), m_toggleUpgradeMapper, SLOT(map()));
        ui->treeWidgetClickTargets->setIndexWidget(model->index(i,3), box);

        QPushButton* push = new QPushButton(tr("Update"));
        m_updateMapper->setMapping(push,i);
        connect(push,SIGNAL(clicked()),m_updateMapper,SLOT(map()));
        ui->treeWidgetClickTargets->setIndexWidget(model->index(i,4), push);

        push = new QPushButton(tr("Maintain"));
        m_maintainMapper->setMapping(push,i);
        connect(push,SIGNAL(clicked()),m_maintainMapper,SLOT(map()));
        ui->treeWidgetClickTargets->setIndexWidget(model->index(i,5), push);

        push = new QPushButton(tr("Delete"));
        m_deleteMapper->setMapping(push,i);
        connect(push,SIGNAL(clicked()),m_deleteMapper,SLOT(map()));
        ui->treeWidgetClickTargets->setIndexWidget(model->index(i,6), push);
    }
}

}}
