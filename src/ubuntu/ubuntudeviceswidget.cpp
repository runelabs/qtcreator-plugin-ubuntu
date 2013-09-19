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

#include "ubuntudeviceswidget.h"
#include "ui_ubuntudeviceswidget.h"


#include <coreplugin/modemanager.h>
#include "ubuntuconstants.h"

#include <QDebug>
#include <QSettings>
#include <QDir>


UbuntuDevicesWidget *UbuntuDevicesWidget::m_instance = 0;

UbuntuDevicesWidget *UbuntuDevicesWidget::instance()
{
    return m_instance;
}

UbuntuDevicesWidget::UbuntuDevicesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuDevicesWidget)
{
    ui->setupUi(this);

    m_instance = this;
    m_deviceDetected = false;
    m_aboutToClose = false;
    //ui->widgetDeviceInfo->hide();
    ui->widgetSshProperties->hide();
    ui->pushButtonSshInstall->hide();
    ui->pushButtonSshRemove->hide();
    //hide();
    //ui->pushButtonCancel->hide();
    ui->frameNoDevices->hide();
    ui->lblLoading->hide();
    ui->frameNoNetwork->hide();

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);

    connect(&m_ubuntuDeviceNotifier,SIGNAL(deviceConnected(QString)),this,SLOT(onDeviceConnected(QString)));
    connect(&m_ubuntuDeviceNotifier,SIGNAL(deviceDisconnected()),this,SLOT(onDeviceDisconnected()));

    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString, int)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));
    detectDevices();
}

void UbuntuDevicesWidget::onMessage(QString msg) {
    m_reply.append(msg);
    ui->plainTextEdit->appendPlainText(msg.trimmed());
}

void UbuntuDevicesWidget::onStarted(QString cmd) {
    ui->stackedWidgetConnectedDevice->setCurrentIndex(1);
    ui->lblDeviceProcessInfo->setText(QFileInfo(cmd).baseName());
    //ui->widgetDeviceInfo->hide();
    //ui->pushButtonCancel->show();
    ui->lblLoading->show();
}


void UbuntuDevicesWidget::onFinished(QString cmd, int code) {
    ui->stackedWidgetConnectedDevice->setCurrentIndex(0);
    if (m_aboutToClose) { return; }

    bool bOk = true;
    bool bHasNetwork = true;

    if (cmd == QString::fromLatin1("%0/device_search").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        QStringList lines = m_reply.trimmed().split(QLatin1String("\n"));

        // fill combobox data
        ui->comboBoxSerialNumber->setEnabled(false);
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.isEmpty()) {
                continue;
            }

            QStringList lineData = line.split(QLatin1String("       "));

            if (lineData.count() == 2) {
                QString sSerialNumber = lineData.takeFirst();
                QString sDeviceInfo = lineData.takeFirst();
                ui->comboBoxSerialNumber->addItem(sSerialNumber.trimmed(),sDeviceInfo);
            }
        }
        ui->comboBoxSerialNumber->setEnabled(true);

        // set serial number value
        m_deviceSerialNumber = ui->comboBoxSerialNumber->currentText();
        m_ubuntuDeviceNotifier.startMonitoring(m_deviceSerialNumber);

        // if there are no devices, or if there is no permission
        if (lines.count() == 0 || m_deviceSerialNumber.isEmpty()  || ui->comboBoxSerialNumber->currentText().startsWith(QLatin1String("???"))) {
            ui->frameNoDevices->show();
            ui->widgetDeviceSerial->hide();
            ui->comboBoxSerialNumber->clear();
            bOk = false;
            m_deviceDetected = false;

            ui->stackedWidgetDeviceConnected->setCurrentIndex(0);
            endAction(QString::fromLatin1(" * there is no device connected."));
        } else if (lines.count() > 0) {
            ui->frameNoDevices->hide();
            ui->widgetDeviceSerial->show();
            m_deviceDetected = true;
            ui->stackedWidgetDeviceConnected->setCurrentIndex(1);
            endAction(QString::fromLatin1(" * found %0 devices.").arg(lines.count()));

            detectDeviceVersion();
        }
        emit updateDeviceActions();

    } else if (cmd == QString::fromLatin1("%0/device_version").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..device version detected."));
        detectHasNetworkConnection();
    } else if (cmd == QString::fromLatin1("%0/openssh_version").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        if (m_reply.trimmed() != QLatin1String("(none)") && m_reply.trimmed() != QLatin1String("")) {
            endAction(QString::fromLatin1("..openssh-server (%0) is installed.").arg(m_reply.trimmed()));
            ui->widgetSshProperties->show();
            ui->pushButtonSshInstall->hide();
            ui->pushButtonSshRemove->show();
            ui->stackedWidgetDeveloperMode->setCurrentIndex(1);
            on_pushButtonPortForward_clicked();
        } else {
            endAction(QString::fromLatin1("..openssh-server was not installed."));
            ui->pushButtonSshInstall->show();
            ui->pushButtonSshRemove->hide();
            ui->widgetSshProperties->hide();
            ui->stackedWidgetDeveloperMode->setCurrentIndex(0);
        }

    } else if (cmd == QString::fromLatin1("%0/qtc_device_developertools").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..platform development was enabled."));
    } else if (cmd == QString::fromLatin1("%0/openssh_remove").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..openssh-server was removed."));
        detectOpenSsh();
    } else if (cmd == QString::fromLatin1("%0/openssh_install").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..openssh-server was installed."));
        detectOpenSsh();
    } else if (cmd == QString::fromLatin1("%0/device_portforward").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..ports forwarded."));
        on_pushButtonSshSetupPublicKey_clicked();
    } else if (cmd == QString::fromLatin1("%0/openssh_publickey").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..public key authentication is now set."));
    } else if (cmd == QString::fromLatin1("%0/device_network_clone").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..network configuration copied."));
        detectHasNetworkConnection();
    } else if (cmd == QString::fromLatin1("%0/device_time_clone").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1("..time configuration copied."));
    } else if (cmd == QString::fromLatin1("%0/device_hasnetwork").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        if (m_reply.trimmed() == QString::fromLatin1("1")) {
            // we have network
            ui->frameNoNetwork->hide();

            detectOpenSsh();
        } else {
            // not set
            bHasNetwork = false;
            ui->frameNoNetwork->show();
            ui->stackedWidgetDeveloperMode->setCurrentIndex(2);
        }
    } else {
        // left empty
    }


    ui->lblLoading->hide();
    m_reply.clear();
}


void UbuntuDevicesWidget::on_pushButtonPlatformDevelopment_clicked() {
    beginAction(QString::fromLatin1("Enable Platform Development.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/qtc_device_developertools %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Enable Platform Development.."));
}

void UbuntuDevicesWidget::on_pushButtonReboot_clicked() {
    beginAction(QString::fromLatin1("Reboot device.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_reboot %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Reboot device.."));
}

void UbuntuDevicesWidget::on_pushButtonShutdown_clicked() {
    beginAction(QString::fromLatin1("Shutdown device.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_shutdown %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Shutdown device.."));
}

void UbuntuDevicesWidget::on_pushButtonRebootToBootloader_clicked() {
    beginAction(QString::fromLatin1("Reboot to bootloader.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_reboot2bootloader %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Reboot to bootloader.."));
}

void UbuntuDevicesWidget::on_pushButtonRebootToRecovery_clicked() {
    beginAction(QString::fromLatin1("Reboot to recovery.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_reboot2recovery %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Reboot to recovery.."));
}

void UbuntuDevicesWidget::detectOpenSsh() {
    beginAction(QString::fromLatin1("Detecting if openssh-server is installed.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/openssh_version %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Detecting openssh-server"));
}

void UbuntuDevicesWidget::detectDevices() {
    beginAction(QString::fromLatin1("Detecting device.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_search").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Searching Ubuntu Touch device"));
}

void UbuntuDevicesWidget::on_pushButtonSshConnect_clicked() {
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/openssh_connect %1 %2 %3").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()).arg(ui->spinBoxSshPort->value()).arg(ui->lineEditUserName->text()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Opening ssh connection to device"));
}

void UbuntuDevicesWidget::on_pushButtonCloneNetworkConfig_clicked() {
    beginAction(QString::fromLatin1("Clone network configuration from host to device.."));
    ui->frameNoNetwork->hide();
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_network_clone %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Clone network configuration from host to device.."));
}

void UbuntuDevicesWidget::on_comboBoxSerialNumber_currentIndexChanged( const QString & text ) {
    m_deviceSerialNumber = text;
    if (!text.isEmpty() && ui->comboBoxSerialNumber->isEnabled()) {
        m_ubuntuDeviceNotifier.startMonitoring(m_deviceSerialNumber);
        ui->lblDeviceInfo->setText(ui->comboBoxSerialNumber->itemData(ui->comboBoxSerialNumber->currentIndex()).toString());

        detectDeviceVersion();
    }
}

void UbuntuDevicesWidget::onError(QString msg) {
    ui->plainTextEdit->appendHtml(QString::fromLatin1("<p style=\"color: red\">%0</p>").arg(msg));
}

void UbuntuDevicesWidget::onDeviceConnected(QString serialNumber) {
    QSettings settings(QLatin1String("Canonical"),QLatin1String("UbuntuSDK"));
    settings.beginGroup(QLatin1String("Devices"));
    if (settings.value(QLatin1String("Auto_Toggle"),true).toBool()) {
        Core::ModeManager::activateMode(Ubuntu::Constants::UBUNTU_MODE_DEVICES);
    }

    m_reply.clear();

    ui->plainTextEdit->clear();

    m_ubuntuProcess.stop();
    ui->comboBoxSerialNumber->clear();
    ui->frameNoDevices->hide();
    ui->frameNoNetwork->hide();
    ui->frameProgress->show();
    ui->lblLoading->show();
    detectDevices();
}

void UbuntuDevicesWidget::onDeviceDisconnected() {

    m_reply.clear();

    ui->plainTextEdit->clear();

     m_ubuntuProcess.stop();
    ui->comboBoxSerialNumber->clear();
    ui->frameNoDevices->hide();
    ui->frameNoNetwork->hide();
    ui->frameProgress->show();
ui->lblLoading->show();
     detectDevices();
}


void UbuntuDevicesWidget::on_pushButtonRefresh_clicked() {
    m_deviceDetected = false;
    m_ubuntuProcess.stop();

    ui->plainTextEdit->clear();

    m_reply.clear();

    ui->frameNoDevices->hide();

    //ui->widgetDeviceInfo->hide();
    ui->lblLoading->show();
 //   ui->pushButtonCancel->show();

    //ui->lblDeviceName->setText(QLatin1String(""));
    ui->comboBoxSerialNumber->clear();
   // ui->lblSerialnumber->setText(QLatin1String(""));

    detectDevices();
}

void UbuntuDevicesWidget::beginAction(QString msg) {
    ui->plainTextEdit->appendHtml(QString::fromLatin1("<p style=\"color: #888\">%0</p>").arg(msg));
}

void UbuntuDevicesWidget::endAction(QString msg) {
    ui->plainTextEdit->appendHtml(QString::fromLatin1("<p style=\"color: #888\">%0</p>").arg(msg));
}

void UbuntuDevicesWidget::on_pushButtonPortForward_clicked() {
    beginAction(QString::fromLatin1("Enabling port forward.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_portforward %1 %2 %3").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()).arg(ui->spinBoxSshPort->value()).arg(ui->spinBoxQmlPort->value()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Enabling port forward.."));
}

void UbuntuDevicesWidget::on_pushButtonSshSetupPublicKey_clicked() {
    beginAction(QString::fromLatin1("Setting up public key authentication.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/openssh_publickey %1 %2").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()).arg(ui->lineEditUserName->text()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Setting up public key authentication.."));
}

void UbuntuDevicesWidget::detectHasNetworkConnection() {
    beginAction(QString::fromLatin1("Check if the device is connected to a network.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_hasnetwork %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Check if the device is connected to a network"));
}

QString UbuntuDevicesWidget::serialNumber() {
    return ui->comboBoxSerialNumber->currentText();
}

void UbuntuDevicesWidget::detectDeviceVersion() {
    beginAction(QString::fromLatin1("Check device image version.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_version %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Check device image version.."));
}

UbuntuDevicesWidget::~UbuntuDevicesWidget()
{
    m_aboutToClose = true;
    m_ubuntuProcess.stop();
    delete ui;
}

void UbuntuDevicesWidget::on_pushButtonSshInstall_clicked() {
    beginAction(QString::fromLatin1("Installing openssh-server.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/openssh_install %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Installing openssh-server.."));
}

void UbuntuDevicesWidget::on_pushButtonCloneTimeConfig_clicked() {
    beginAction(QString::fromLatin1("Cloning time configuration from host to device.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/device_time_clone %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Cloning time configuration from host to device.."));
}

void UbuntuDevicesWidget::on_pushButtonSshRemove_clicked() {
    beginAction(QString::fromLatin1("Removing openssh-server.."));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1("%0/openssh_remove %1").arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1("Removing openssh-server.."));
}
