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
#include "settings.h"

using namespace Ubuntu;

UbuntuSettingsDeviceConnectivityWidget::UbuntuSettingsDeviceConnectivityWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuSettingsDeviceConnectivityWidget)
{
    ui->setupUi(this);

    Internal::Settings::DeviceConnectivity devConn = Internal::Settings::deviceConnectivity();

    ui->lineEditDeviceUserName->setText(devConn.user);
    ui->lineEditDeviceIP->setText(devConn.ip);
    ui->checkBox_mode_devices_autotoggle->setChecked(Internal::Settings::deviceAutoToggle());
}

UbuntuSettingsDeviceConnectivityWidget::~UbuntuSettingsDeviceConnectivityWidget()
{
    delete ui;
}

void UbuntuSettingsDeviceConnectivityWidget::apply() {

    Internal::Settings::DeviceConnectivity devConn = Internal::Settings::deviceConnectivity();
    devConn.user = ui->lineEditDeviceUserName->text();
    devConn.ip   = ui->lineEditDeviceIP->text();
    Internal::Settings::setDeviceConnectivity(devConn);
    Internal::Settings::setDeviceAutoToggle(ui->checkBox_mode_devices_autotoggle->isChecked());
    Internal::Settings::flushSettings();
}
