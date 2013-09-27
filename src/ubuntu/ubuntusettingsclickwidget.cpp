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
#include <QFileDialog>

using namespace Ubuntu;

UbuntuSettingsClickWidget::UbuntuSettingsClickWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuSettingsClickWidget)
{
    ui->setupUi(this);
    m_settings = new QSettings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_CLICK));
    ui->groupBox_4->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS),Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS).toBool());
    ui->lineEditPackagingToolsLocation->setText(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS_LOCATION),QLatin1String(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION)).toString());
    m_settings->endGroup();
}

void UbuntuSettingsClickWidget::apply() {
    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_CLICK));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS),ui->groupBox_4->isChecked());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS_LOCATION),ui->lineEditPackagingToolsLocation->text());
    m_settings->endGroup();

    m_settings->sync();
}

UbuntuSettingsClickWidget::~UbuntuSettingsClickWidget()
{
    delete ui;
}

void UbuntuSettingsClickWidget::on_pushButtonFindClickPackagingTools_clicked() {
    QString path = QFileDialog::getExistingDirectory(this,QLatin1String(Constants::UBUNTUSETTINGSCLICKWIDGET_FILEDIALOG));
    if (path.isEmpty()) return;
    ui->lineEditPackagingToolsLocation->setText(path);
}
