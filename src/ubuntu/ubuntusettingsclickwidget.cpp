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
#include <QFileDialog>
#include <QDir>
#include <QRegExp>
#include <QTreeWidgetItem>
#include <QDebug>

using namespace Ubuntu;

enum {
    debug = 0
};

UbuntuSettingsClickWidget::UbuntuSettingsClickWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuSettingsClickWidget)
{
    ui->setupUi(this);

    m_settings = new QSettings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_CLICK));
    ui->enableUpdateCheckerCheckBox->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_AUTO_CHECK_CHROOT_UPDATES),true).toBool());
    ui->checkBoxLocalMirror->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_CHROOT_USE_LOCAL_MIRROR),false).toBool());
    m_settings->endGroup();

    m_deleteMapper = new QSignalMapper(this);
    connect(m_deleteMapper, SIGNAL(mapped(int)),this, SLOT(on_deleteClickChroot(int)));
    m_maintainMapper = new QSignalMapper(this);
    connect(m_maintainMapper, SIGNAL(mapped(int)),this, SLOT(on_maintainClickChroot(int)));
    m_updateMapper = new QSignalMapper(this);
    connect(m_updateMapper, SIGNAL(mapped(int)),this, SLOT(on_upgradeClickChroot(int)));

    QStringList headers;
    headers << tr("Series")<< tr("Framework") << tr("Architecture")<<QLatin1String("")<<QLatin1String("")<<QLatin1String("");
    ui->treeWidgetClickTargets->setHeaderLabels(headers);
    listExistingClickTargets();
}

void UbuntuSettingsClickWidget::apply() {

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_CLICK));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_AUTO_CHECK_CHROOT_UPDATES),ui->enableUpdateCheckerCheckBox->checkState() == Qt::Checked);
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_CHROOT_USE_LOCAL_MIRROR),ui->checkBoxLocalMirror->checkState() == Qt::Checked);
    m_settings->endGroup();

    m_settings->sync();
}

UbuntuSettingsClickWidget::~UbuntuSettingsClickWidget()
{
    delete ui;
}

void UbuntuSettingsClickWidget::on_pushButtonCreateClickTarget_clicked()
{
   QString dummy;
   Internal::UbuntuClickDialog::createClickChrootModal(true, dummy, this);
    listExistingClickTargets();
}

void UbuntuSettingsClickWidget::on_deleteClickChroot(const int index)
{
    if(index < 0 || index > m_availableTargets.size())
        return;

    if(debug) qDebug()<<"Destroying chroot "<< m_availableTargets.at(index);

    Internal::UbuntuClickDialog::maintainClickModal(m_availableTargets.at(index),Internal::UbuntuClickTool::Delete);
    listExistingClickTargets();
}

void UbuntuSettingsClickWidget::on_maintainClickChroot(const int index)
{
    if(index < 0 || index > m_availableTargets.size())
        return;
    Internal::UbuntuClickTool::openChrootTerminal(m_availableTargets.at(index));
}

void UbuntuSettingsClickWidget::on_upgradeClickChroot(const int index)
{
    if(index < 0 || index > m_availableTargets.size())
        return;
    Internal::UbuntuClickDialog::maintainClickModal(m_availableTargets.at(index),Internal::UbuntuClickTool::Upgrade);
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

    QList<Internal::UbuntuClickTool::Target> items = Internal::UbuntuClickTool::listAvailableTargets();
    m_availableTargets = items;

    QAbstractItemModel* model = ui->treeWidgetClickTargets->model();

    //fill the treeview with all existing chroots
    for(int i = 0; i < items.size(); i++) {
        const Internal::UbuntuClickTool::Target& target = items.at(i);

        QTreeWidgetItem* chrootItem = new QTreeWidgetItem;
        chrootItem->setText(0,target.series);
        chrootItem->setText(1,target.framework);
        chrootItem->setText(2,target.architecture);

        ui->treeWidgetClickTargets->addTopLevelItem(chrootItem);

        QPushButton* push = new QPushButton(tr("Update"));
        m_updateMapper->setMapping(push,i);
        connect(push,SIGNAL(clicked()),m_updateMapper,SLOT(map()));
        ui->treeWidgetClickTargets->setIndexWidget(model->index(i,3), push);

        push = new QPushButton(tr("Maintain"));
        m_maintainMapper->setMapping(push,i);
        connect(push,SIGNAL(clicked()),m_maintainMapper,SLOT(map()));
        ui->treeWidgetClickTargets->setIndexWidget(model->index(i,4), push);

        push = new QPushButton(tr("Delete"));
        m_deleteMapper->setMapping(push,i);
        connect(push,SIGNAL(clicked()),m_deleteMapper,SLOT(map()));
        ui->treeWidgetClickTargets->setIndexWidget(model->index(i,5), push);
    }
}
