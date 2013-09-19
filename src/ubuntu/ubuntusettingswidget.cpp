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

#include "ubuntusettingswidget.h"
#include "ui_ubuntusettingswidget.h"

UbuntuSettingsWidget::UbuntuSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuSettingsWidget)
{
    ui->setupUi(this);
    m_settings = new QSettings(QLatin1String("Canonical"),QLatin1String("UbuntuSDK"));
    m_settings->beginGroup(QLatin1String("Mode"));
    ui->checkBox_mode_api->setChecked(m_settings->value(QLatin1String("API"),true).toBool());
    ui->checkBox_mode_coreapps->setChecked(m_settings->value(QLatin1String("CoreApps"),true).toBool());
    ui->checkBox_mode_irc->setChecked(m_settings->value(QLatin1String("IRC"),true).toBool());
    ui->checkBox_mode_pastebin->setChecked(m_settings->value(QLatin1String("Pastebin"),true).toBool());
    ui->checkBox_mode_wiki->setChecked(m_settings->value(QLatin1String("Wiki"),true).toBool());
    m_settings->endGroup();
    m_settings->beginGroup(QLatin1String("Devices"));
    ui->checkBox_mode_devices_autotoggle->setChecked(m_settings->value(QLatin1String("Auto_Toggle"),true).toBool());
    m_settings->endGroup();
}

void UbuntuSettingsWidget::apply() {
    m_settings->beginGroup(QLatin1String("Mode"));
    m_settings->setValue(QLatin1String("API"),ui->checkBox_mode_api->isChecked());
    m_settings->setValue(QLatin1String("CoreApps"),ui->checkBox_mode_coreapps->isChecked());
    m_settings->setValue(QLatin1String("IRC"),ui->checkBox_mode_irc->isChecked());
    m_settings->setValue(QLatin1String("Pastebin"),ui->checkBox_mode_pastebin->isChecked());
    m_settings->setValue(QLatin1String("Wiki"),ui->checkBox_mode_wiki->isChecked());
    m_settings->endGroup();
    m_settings->beginGroup(QLatin1String("Devices"));
    m_settings->setValue(QLatin1String("Auto_Toggle"),ui->checkBox_mode_devices_autotoggle->isChecked());
    m_settings->endGroup();
    m_settings->sync();
}

UbuntuSettingsWidget::~UbuntuSettingsWidget()
{
    delete ui;
}
