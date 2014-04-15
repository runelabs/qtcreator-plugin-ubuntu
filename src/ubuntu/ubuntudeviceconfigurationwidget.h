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
#ifndef UBUNTU_INTERNAL_UBUNTUDEVICECONFIGURATIONWIDGET_H
#define UBUNTU_INTERNAL_UBUNTUDEVICECONFIGURATIONWIDGET_H

#include <projectexplorer/devicesupport/idevicewidget.h>
#include "ubuntudevice.h"

namespace Ubuntu {
namespace Internal {

namespace Ui {
class UbuntuDeviceConfigurationWidget;
}

class UbuntuDeviceConfigurationWidget : public ProjectExplorer::IDeviceWidget
{
    Q_OBJECT

public:
    explicit UbuntuDeviceConfigurationWidget(const ProjectExplorer::IDevice::Ptr &deviceConfig, QWidget *parent = 0);
    ~UbuntuDeviceConfigurationWidget();

    // IDeviceWidget interface
    virtual void updateDeviceFromUi();

private slots:
    void updateWidgets();
    void on_message (const QString &message);
    void on_pushButtonSshInstall_clicked();
    void on_pushButtonSshRemove_clicked();
    void on_pushButtonCloneTimeConfig_clicked();
    void on_pushButtonPortForward_clicked();
    void on_pushButtonSshSetupPublicKey_clicked();
    void on_pushButtonSshConnect_clicked();
    void on_pushButton_filesystem_rw_disable_clicked();
    void on_pushButton_filesystem_rw_enable_clicked();
    void on_pushButtonPlatformDevelopmentRemove_clicked();
    void on_pushButtonPlatformDevelopment_clicked();
    void on_pushButtonReboot_clicked();
    void on_pushButtonShutdown_clicked();
    void on_pushButtonRebootToBootloader_clicked();
    void on_pushButtonRebootToRecovery_clicked();

private:
    Ui::UbuntuDeviceConfigurationWidget *ui;
    UbuntuDevice::Ptr m_ubuntuDev;
};


} // namespace Internal
} // namespace Ubuntu
#endif // UBUNTU_INTERNAL_UBUNTUDEVICECONFIGURATIONWIDGET_H
