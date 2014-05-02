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

#include "ubuntusettingsdeviceconnectivitywidget.h"
#include "ui_ubuntusettingsdeviceconnectivitywidget.h"
#include "ubuntuconstants.h"

using namespace Ubuntu;

UbuntuSettingsDeviceConnectivityWidget::UbuntuSettingsDeviceConnectivityWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuSettingsDeviceConnectivityWidget)
{
    ui->setupUi(this);
    m_settings = new QSettings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    ui->lineEditDeviceUserName->setText(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_USERNAME),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_USERNAME)).toString());
    ui->lineEditDeviceIP->setText(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_IP),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_IP)).toString());
    m_settings->endGroup();

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICES));
    ui->checkBox_mode_devices_autotoggle->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_AUTOTOGGLE),Constants::SETTINGS_DEFAULT_DEVICES_AUTOTOGGLE).toBool());
    m_settings->endGroup();
}

UbuntuSettingsDeviceConnectivityWidget::~UbuntuSettingsDeviceConnectivityWidget()
{
    delete ui;
}

void UbuntuSettingsDeviceConnectivityWidget::apply() {
    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_USERNAME),ui->lineEditDeviceUserName->text());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_IP),ui->lineEditDeviceIP->text());
    m_settings->endGroup();

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICES));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_AUTOTOGGLE),ui->checkBox_mode_devices_autotoggle->isChecked());
    m_settings->endGroup();

    m_settings->sync();
}
