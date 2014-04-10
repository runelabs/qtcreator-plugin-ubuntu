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
#include "ubuntudeviceconfigurationwidget.h"
#include "ui_ubuntudeviceconfigurationwidget.h"

namespace Ubuntu {
namespace Internal {

UbuntuDeviceConfigurationWidget::UbuntuDeviceConfigurationWidget(const ProjectExplorer::IDevice::Ptr &deviceConfig, QWidget *parent)
    : ProjectExplorer::IDeviceWidget(deviceConfig, parent)
    , ui(new Ui::UbuntuDeviceConfigurationWidget)
{
    ui->setupUi(this);

    m_ubuntuDev = qSharedPointerCast<UbuntuDevice>(this->device());
    connect(m_ubuntuDev->helper(),SIGNAL(featureDetected()),this,SLOT(updateWidgets()));
    connect(m_ubuntuDev->helper(),SIGNAL(disconnected()),this,SLOT(updateWidgets()));
    connect(m_ubuntuDev->helper(),SIGNAL(connected()),this,SLOT(updateWidgets()));
    connect(m_ubuntuDev->helper(),SIGNAL(message(QString)),this,SLOT(on_message(QString)));

    ui->plainTextEdit->appendHtml(m_ubuntuDev->helper()->log());

    updateWidgets();
}

UbuntuDeviceConfigurationWidget::~UbuntuDeviceConfigurationWidget()
{
    delete ui;
}

void UbuntuDeviceConfigurationWidget::updateDeviceFromUi()
{
    //write settings to device
}

void UbuntuDeviceConfigurationWidget::updateWidgets()
{
    if(m_ubuntuDev->detectionState() != UbuntuDevice::Done) {
        ui->pushButtonSshInstall->setDisabled(true);
        ui->pushButtonSshRemove->setVisible(false);

        ui->pushButtonPortForward->setDisabled(true);
        ui->pushButtonSshSetupPublicKey->setDisabled(true);
        ui->pushButtonSshConnect->setDisabled(true);

        ui->pushButtonPlatformDevelopment->setDisabled(true);
        ui->pushButtonPlatformDevelopment->setVisible(true);
        ui->pushButtonPlatformDevelopmentRemove->setDisabled(true);
        ui->pushButtonPlatformDevelopmentRemove->setVisible(false);
        ui->pushButton_filesystem_rw_enable->setDisabled(true);
        ui->pushButton_filesystem_rw_disable->setVisible(false);
    } else {

        bool nwAvailable = m_ubuntuDev->hasNetworkConnection() == UbuntuDevice::Available;

        //Setup SSH relevant buttons
        bool sshUnknown   = m_ubuntuDev->developerModeEnabled() == UbuntuDevice::Unknown;
        bool sshAvailable = (m_ubuntuDev->developerModeEnabled() == UbuntuDevice::Available);
        ui->pushButtonSshInstall->setVisible(!sshAvailable);
        ui->pushButtonSshInstall->setEnabled(!sshAvailable && !sshUnknown && nwAvailable);
        ui->pushButtonSshRemove->setVisible(sshAvailable && !sshUnknown);
        ui->pushButtonSshRemove->setEnabled(sshAvailable);
        ui->pushButtonSshSetupPublicKey->setEnabled(sshAvailable);
        ui->pushButtonSshConnect->setEnabled(sshAvailable);
        ui->pushButtonPortForward->setEnabled(sshAvailable);

        //Writeable Filesystem
        bool writeableImageAvailable = m_ubuntuDev->hasWriteableImage() == UbuntuDevice::Available;
        bool writeableImageUnknown   = m_ubuntuDev->hasWriteableImage() == UbuntuDevice::Unknown;
        ui->pushButton_filesystem_rw_enable->setEnabled(!writeableImageAvailable && !writeableImageUnknown);
        ui->pushButton_filesystem_rw_enable->setVisible(!writeableImageAvailable);
        ui->pushButton_filesystem_rw_disable->setVisible(writeableImageAvailable);
        ui->pushButton_filesystem_rw_disable->setEnabled(writeableImageAvailable);

        //Platform Development
        bool platformDevAvailable = m_ubuntuDev->hasDeveloperTools() == UbuntuDevice::Available;
        bool platformDevUnknown   = m_ubuntuDev->hasDeveloperTools() == UbuntuDevice::Unknown;
        ui->pushButtonPlatformDevelopment->setEnabled(!platformDevAvailable && !platformDevUnknown && nwAvailable);
        ui->pushButtonPlatformDevelopment->setVisible(!platformDevAvailable);
        ui->pushButtonPlatformDevelopmentRemove->setEnabled(platformDevAvailable);
        ui->pushButtonPlatformDevelopmentRemove->setVisible(platformDevAvailable);
    }

    bool connected = (m_ubuntuDev->deviceState() != ProjectExplorer::IDevice::DeviceDisconnected);
    ui->pushButtonReboot->setEnabled(connected);
    ui->pushButtonRebootToBootloader->setEnabled(connected);
    ui->pushButtonRebootToRecovery->setEnabled(connected);
    ui->pushButtonShutdown->setEnabled(connected);
    ui->pushButtonCloneTimeConfig->setEnabled(connected);
}

void UbuntuDeviceConfigurationWidget::on_message(const QString &message)
{
    ui->plainTextEdit->appendHtml(message);
}

void UbuntuDeviceConfigurationWidget::on_pushButtonSshInstall_clicked()
{
    ui->pushButtonSshInstall->setEnabled(false);
    m_ubuntuDev->setDeveloperModeEnabled(true);
}

void UbuntuDeviceConfigurationWidget::on_pushButtonSshRemove_clicked()
{
    ui->pushButtonSshRemove->setEnabled(false);
    m_ubuntuDev->setDeveloperModeEnabled(false);
}

void UbuntuDeviceConfigurationWidget::on_pushButtonCloneTimeConfig_clicked()
{
    ui->pushButtonCloneTimeConfig->setEnabled(false);
    m_ubuntuDev->cloneTimeConfig();
}

void UbuntuDeviceConfigurationWidget::on_pushButtonPortForward_clicked()
{
    ui->pushButtonPortForward->setEnabled(false);
    m_ubuntuDev->enablePortForward();
}

void UbuntuDeviceConfigurationWidget::on_pushButtonSshSetupPublicKey_clicked()
{
    ui->pushButtonSshSetupPublicKey->setEnabled(false);
    m_ubuntuDev->deployPublicKey();
}

void UbuntuDeviceConfigurationWidget::on_pushButtonSshConnect_clicked()
{
    m_ubuntuDev->openTerminal();
}

void UbuntuDeviceConfigurationWidget::on_pushButton_filesystem_rw_disable_clicked()
{
    ui->pushButton_filesystem_rw_disable->setEnabled(false);
    m_ubuntuDev->setWriteableImageEnabled(false);
}

void UbuntuDeviceConfigurationWidget::on_pushButton_filesystem_rw_enable_clicked()
{
    ui->pushButton_filesystem_rw_enable->setEnabled(false);
    m_ubuntuDev->setWriteableImageEnabled(true);
}

void UbuntuDeviceConfigurationWidget::on_pushButtonPlatformDevelopmentRemove_clicked()
{
    ui->pushButtonPlatformDevelopmentRemove->setEnabled(false);
    m_ubuntuDev->setDeveloperToolsInstalled(false);
}

void UbuntuDeviceConfigurationWidget::on_pushButtonPlatformDevelopment_clicked()
{
    ui->pushButtonPlatformDevelopment->setEnabled(false);
    m_ubuntuDev->setDeveloperToolsInstalled(true);
}

void UbuntuDeviceConfigurationWidget::on_pushButtonReboot_clicked()
{
    ui->pushButtonReboot->setEnabled(false);
    m_ubuntuDev->reboot();
}

void UbuntuDeviceConfigurationWidget::on_pushButtonShutdown_clicked()
{
    ui->pushButtonShutdown->setEnabled(false);
    m_ubuntuDev->shutdown();
}

void UbuntuDeviceConfigurationWidget::on_pushButtonRebootToBootloader_clicked()
{
    ui->pushButtonRebootToBootloader->setEnabled(false);
    m_ubuntuDev->rebootToBootloader();
}

void UbuntuDeviceConfigurationWidget::on_pushButtonRebootToRecovery_clicked()
{
    ui->pushButtonRebootToRecovery->setEnabled(false);
    m_ubuntuDev->rebootToRecovery();
}

} // namespace Internal
} // namespace Ubuntu
