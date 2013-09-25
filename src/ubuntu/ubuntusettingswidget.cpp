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
#include "ubuntuconstants.h"

using namespace Ubuntu;

UbuntuSettingsWidget::UbuntuSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuSettingsWidget)
{
    ui->setupUi(this);
    m_settings = new QSettings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_MODE));
    ui->checkBox_mode_api->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_API),Constants::SETTINGS_DEFAULT_API_VISIBILITY).toBool());
    ui->checkBox_mode_coreapps->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_COREAPPS),Constants::SETTINGS_DEFAULT_COREAPPS_VISIBILITY).toBool());
    ui->checkBox_mode_irc->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_IRC),Constants::SETTINGS_DEFAULT_IRC_VISIBILITY).toBool());
    ui->checkBox_mode_pastebin->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_PASTEBIN),Constants::SETTINGS_DEFAULT_PASTEBIN_VISIBILITY).toBool());
    ui->checkBox_mode_wiki->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_WIKI),Constants::SETTINGS_DEFAULT_WIKI_VISIBILITY).toBool());
    m_settings->endGroup();

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    ui->lineEditDeviceUserName->setText(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_USERNAME),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_USERNAME)).toString());
    ui->lineEditDeviceIP->setText(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_IP),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_IP)).toString());
    ui->spinBoxDeviceQmlPort->setValue(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_QML),Constants::SETTINGS_DEFAULT_DEVICE_QML_PORT).toInt());
    ui->spinBoxDeviceSshPort->setValue(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_SSH),Constants::SETTINGS_DEFAULT_DEVICE_SSH_PORT).toInt());
    m_settings->endGroup();

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICES));
    ui->checkBox_mode_devices_autotoggle->setChecked(m_settings->value(QLatin1String(Constants::SETTINGS_KEY_AUTOTOGGLE),Constants::SETTINGS_DEFAULT_DEVICES_AUTOTOGGLE).toBool());
    m_settings->endGroup();
}

void UbuntuSettingsWidget::apply() {
    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_MODE));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_API),ui->checkBox_mode_api->isChecked());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_COREAPPS),ui->checkBox_mode_coreapps->isChecked());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_IRC),ui->checkBox_mode_irc->isChecked());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_PASTEBIN),ui->checkBox_mode_pastebin->isChecked());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_WIKI),ui->checkBox_mode_wiki->isChecked());
    m_settings->endGroup();

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_USERNAME),ui->lineEditDeviceUserName->text());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_IP),ui->lineEditDeviceIP->text());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_QML),ui->spinBoxDeviceQmlPort->value());
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_SSH),ui->spinBoxDeviceSshPort->value());
    m_settings->endGroup();

    m_settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICES));
    m_settings->setValue(QLatin1String(Constants::SETTINGS_KEY_AUTOTOGGLE),ui->checkBox_mode_devices_autotoggle->isChecked());
    m_settings->endGroup();
    m_settings->sync();
}

UbuntuSettingsWidget::~UbuntuSettingsWidget()
{
    delete ui;
}
