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

#include "ubuntusecuritypolicypickerdialog.h"
#include "ui_ubuntusecuritypolicypickerdialog.h"

UbuntuSecurityPolicyPickerDialog::UbuntuSecurityPolicyPickerDialog(const QString &policyVersion, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UbuntuSecurityPolicyPickerDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint | Qt::Popup);
    m_model.setPolicyVersion(policyVersion);
    m_model.scanPolicyGroups();
    connect(&m_model,SIGNAL(scanComplete(bool)),this,SLOT(onScanComplete(bool)));

    ui->listViewPolicyGroups->setModel(&m_model);
    ui->stackedWidget->setCurrentIndex(0);

    QItemSelectionModel *selModel = ui->listViewPolicyGroups->selectionModel();
    connect(selModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(onPolicyClicked(QModelIndex)));

    connect(&m_info,SIGNAL(infoReady(bool)),this,SLOT(onInfoChanged(bool)));
}

UbuntuSecurityPolicyPickerDialog::~UbuntuSecurityPolicyPickerDialog()
{
    delete ui;
}

void UbuntuSecurityPolicyPickerDialog::onScanComplete(bool ok) {
    if (ok) {
        ui->stackedWidget->setCurrentIndex(1);
        ui->listViewPolicyGroups->selectionModel()->setCurrentIndex(m_model.index(0,0),
                                                                    QItemSelectionModel::SelectCurrent);
        ui->listViewPolicyGroups->setFocus();
    } else {
        ui->stackedWidget->setCurrentIndex(2);
    }
}

QStringList UbuntuSecurityPolicyPickerDialog::selectedPolicyGroups() {
    QStringList retval;
    if (ui->stackedWidget->currentIndex()==2) {
        retval.append(ui->lineEditPolicyGroup->text());
    } else {
        QModelIndexList selected = ui->listViewPolicyGroups->selectionModel()->selectedIndexes();
        foreach (QModelIndex idx, selected) {
            retval.append(m_model.data(idx,Qt::DisplayRole).toString());
        }
    }
    return retval;
}

void UbuntuSecurityPolicyPickerDialog::on_pushButtonCancel_clicked() {
    this->reject();
}

void UbuntuSecurityPolicyPickerDialog::on_pushButtonAdd_clicked() {
    this->accept();
}

void UbuntuSecurityPolicyPickerDialog::onPolicyClicked(QModelIndex idx) {
    m_info.getInfo(m_model.data(idx,Qt::DisplayRole).toString(),m_model.policyVersion());
}

void UbuntuSecurityPolicyPickerDialog::onInfoChanged(bool ok) {
    if (ok) {
        QString policyGroupInfo = m_info.info();
        policyGroupInfo = policyGroupInfo.replace(QRegExp(QLatin1String("#([^\n]*)")), QLatin1String("<span style=\"color: green\">#\\1</span>"));
        policyGroupInfo = policyGroupInfo.replace(QRegExp(QLatin1String("(# Usage: reserved)"),Qt::CaseInsensitive), QLatin1String("<span style=\"background: red; color: white\">\\1</span>"));
        policyGroupInfo = policyGroupInfo.replace(QRegExp(QLatin1String("\n")), QLatin1String("<br />"));
        ui->textBrowserEditInfo->setHtml(policyGroupInfo);

        ui->textBrowserEditInfo->show();
    } else {
        ui->textBrowserEditInfo->hide();
    }
}
