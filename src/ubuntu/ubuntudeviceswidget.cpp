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

using namespace Ubuntu;

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
    ui->tabWidget_2->setCurrentIndex(0);

    m_instance = this;
    m_deviceDetected = false;
    m_aboutToClose = false;
    ui->widgetSshProperties->hide();
    ui->pushButtonSshInstall->hide();
    ui->pushButtonSshRemove->hide();
    ui->widgetMovedToSettings->hide();

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


UbuntuDevicesWidget::~UbuntuDevicesWidget()
{
    m_aboutToClose = true;
    m_ubuntuProcess.stop();
    delete ui;
}


void UbuntuDevicesWidget::onMessage(QString msg) {
    m_reply.append(msg);
    ui->plainTextEdit->appendPlainText(msg.trimmed());
}

void UbuntuDevicesWidget::onStarted(QString cmd) {
    ui->stackedWidgetConnectedDevice->setCurrentIndex(1);
    ui->lblDeviceProcessInfo->setText(QFileInfo(cmd).baseName());
    ui->lblLoading->show();
}


void UbuntuDevicesWidget::onFinished(QString cmd, int code) {
    Q_UNUSED(code);

    ui->stackedWidgetConnectedDevice->setCurrentIndex(0);
    if (m_aboutToClose) { return; }

    bool bOk = true;
    bool bHasNetwork = true;

    if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVICESEARCH).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));

        // fill combobox data
        ui->comboBoxSerialNumber->setEnabled(false);
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.isEmpty()) {
                continue;
            }

            QStringList lineData = line.split(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_ADB_SEPARATOR));

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
        if (lines.count() == 0 || m_deviceSerialNumber.isEmpty()  || ui->comboBoxSerialNumber->currentText().startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_ADB_NOACCESS))) {
            ui->frameNoDevices->show();
            ui->widgetDeviceSerial->hide();
            ui->comboBoxSerialNumber->clear();
            bOk = false;
            m_deviceDetected = false;

            ui->stackedWidgetDeviceConnected->setCurrentIndex(0);
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NO_DEVICE));
        } else if (lines.count() > 0) {
            ui->frameNoDevices->hide();
            ui->widgetDeviceSerial->show();
            m_deviceDetected = true;
            ui->stackedWidgetDeviceConnected->setCurrentIndex(1);
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_FOUND_DEVICES).arg(lines.count()));

            detectDeviceVersion();
        }
        emit updateDeviceActions();

    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVICEVERSION).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVICEVERSION_DETECTED));
        detectHasNetworkConnection();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_SSH_START).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_STARTED));
        on_pushButtonPortForward_clicked();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_SSH_VERSION).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        if (m_reply.trimmed() != QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NONE) && m_reply.trimmed() != QLatin1String(Constants::EMPTY)) {
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_IS_INSTALLED).arg(m_reply.trimmed()));
            ui->widgetSshProperties->show();
            ui->pushButtonSshInstall->hide();
            ui->pushButtonSshRemove->show();
            ui->stackedWidgetDeveloperMode->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_DEVELOPERMODE_PAGE_ENABLED);
            startSshService();
        } else {
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_NOT_INSTALLED));
            ui->pushButtonSshInstall->show();
            ui->pushButtonSshRemove->hide();
            ui->widgetSshProperties->hide();
            ui->stackedWidgetDeveloperMode->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_DEVELOPERMODE_PAGE_DEVICEFOUND);
        }
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVICE_WRITABLE_SET).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_ENABLED));
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVICE_WRITABLE_UNSET).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_DISABLED));
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVELOPERTOOLS_REMOVED).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_REMOVED));
        detectDeveloperTools();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVELOPERTOOLS_HAS).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        if (m_reply.trimmed() == QLatin1String(Constants::ZERO_STR)) {
            ui->pushButtonPlatformDevelopment->show();
            ui->pushButtonPlatformDevelopmentRemove->hide();
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_NOT_INSTALLED));
        } else {
            ui->pushButtonPlatformDevelopment->hide();
            ui->pushButtonPlatformDevelopmentRemove->show();
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_INSTALLED));
        }
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_HAS).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        if (m_reply.trimmed() == QLatin1String(Constants::ZERO_STR)) {
            ui->pushButton_filesystem_rw_enable->hide();
            ui->pushButton_filesystem_rw_disable->show();
            ui->pushButtonPlatformDevelopment->setEnabled(true);
            ui->pushButtonPlatformDevelopmentRemove->setEnabled(true);
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLEIMAGE));
        } else {
            ui->pushButton_filesystem_rw_enable->show();
            ui->pushButton_filesystem_rw_disable->hide();
            ui->pushButtonPlatformDevelopment->setEnabled(false);
            ui->pushButtonPlatformDevelopmentRemove->setEnabled(false);
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_READONLYIMAGE));
        }
        ui->pushButtonPlatformDevelopment->hide();
        ui->pushButtonPlatformDevelopmentRemove->hide();
        detectDeveloperTools();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVELOPERTOOLS_INSTALL).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_WAS_INSTALLED));
        detectDeveloperTools();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_SSH_REMOVE).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_REMOVED));
        detectOpenSsh();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_SSH_INSTALL).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_INSTALLED));
        detectOpenSsh();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_PORTFORWARD).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_PORTS_FORWARDED));
        on_pushButtonSshSetupPublicKey_clicked();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_PUBLICKEY).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_PUBLICKEY_AUTH_SET));
        detectDeviceWritableImage();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_NETWORKCLONE).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NETWORK_CONF_COPIED));
        detectHasNetworkConnection();
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_TIMECLONE).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_TIME_CONF_COPIED));
    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_HASNETWORK).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        if (m_reply.trimmed() == QString::fromLatin1(Constants::ONE_STR)) {
            // we have network
            ui->frameNoNetwork->hide();

            detectOpenSsh();
        } else {
            // not set
            bHasNetwork = false;
            ui->frameNoNetwork->show();
            ui->stackedWidgetDeveloperMode->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_DEVELOPERMODE_PAGE_NONETWORK);
        }
    } else {
        // left empty
    }

    ui->lblLoading->hide();
    m_reply.clear();
}

void UbuntuDevicesWidget::startSshService() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE));
}

void UbuntuDevicesWidget::on_pushButton_filesystem_rw_enable_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSWRITABLE));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSWRITABLE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSWRITABLE));
}

void UbuntuDevicesWidget::on_pushButton_filesystem_rw_disable_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSREADONLY));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSREADONLY_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSREADONLY));
}

void UbuntuDevicesWidget::detectDeveloperTools() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS));
}

void UbuntuDevicesWidget::detectDeviceWritableImage() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE));
}

void UbuntuDevicesWidget::on_pushButtonPlatformDevelopmentRemove_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT));
}

void UbuntuDevicesWidget::on_pushButtonPlatformDevelopment_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT));
}

void UbuntuDevicesWidget::on_pushButtonReboot_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT));
}

void UbuntuDevicesWidget::on_pushButtonShutdown_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SHUTDOWN));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SHUTDOWN_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SHUTDOWN));
}

void UbuntuDevicesWidget::on_pushButtonRebootToBootloader_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_BOOTLOADER));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_BOOTLOADER_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_BOOTLOADER));
}

void UbuntuDevicesWidget::on_pushButtonRebootToRecovery_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_RECOVERY));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_RECOVERY_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_RECOVERY));
}

void UbuntuDevicesWidget::detectOpenSsh() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTOPENSSH));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTOPENSSH_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTOPENSSH));
}

void UbuntuDevicesWidget::detectDevices() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES));
}

void UbuntuDevicesWidget::on_pushButtonSshConnect_clicked() {
    m_ubuntuProcess.stop();
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    QString deviceUsername = settings.value(QLatin1String(Constants::SETTINGS_KEY_USERNAME),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_USERNAME)).toString();
    QString deviceIp = settings.value(QLatin1String(Constants::SETTINGS_KEY_IP),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_IP)).toString();
    QString devicePort = settings.value(QLatin1String(Constants::SETTINGS_KEY_SSH),Constants::SETTINGS_DEFAULT_DEVICE_SSH_PORT).toString();

    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSHCONNECT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()).arg(devicePort).arg(deviceUsername).arg(deviceIp) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSHCONNECT));
}

void UbuntuDevicesWidget::on_pushButtonCloneNetworkConfig_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK));
    ui->frameNoNetwork->hide();
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK));
}

void UbuntuDevicesWidget::on_pushButtonPortForward_clicked() {
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    QString deviceQmlPort = settings.value(QLatin1String(Constants::SETTINGS_KEY_QML),Constants::SETTINGS_DEFAULT_DEVICE_QML_PORT).toString();
    QString deviceSshPort = settings.value(QLatin1String(Constants::SETTINGS_KEY_SSH),Constants::SETTINGS_DEFAULT_DEVICE_SSH_PORT).toString();

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_PORTFORWARD));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_PORTFORWARD_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()).arg(deviceSshPort).arg(deviceQmlPort) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_PORTFORWARD));
}

void UbuntuDevicesWidget::on_pushButtonSshSetupPublicKey_clicked() {
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    QString deviceUsername = settings.value(QLatin1String(Constants::SETTINGS_KEY_USERNAME),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_USERNAME)).toString();

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()).arg(deviceUsername) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH));
}

void UbuntuDevicesWidget::detectHasNetworkConnection() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_HASNETWORK));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_HASNETWORK_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_HASNETWORK));
}

void UbuntuDevicesWidget::detectDeviceVersion() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION));
}

void UbuntuDevicesWidget::on_pushButtonSshInstall_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL));
}

void UbuntuDevicesWidget::on_pushButtonCloneTimeConfig_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONETIME));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONETIME_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONETIME));
}

void UbuntuDevicesWidget::on_pushButtonSshRemove_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_REMOVE));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_REMOVE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_REMOVE));
}


QString UbuntuDevicesWidget::serialNumber() {
    return ui->comboBoxSerialNumber->currentText();
}

void UbuntuDevicesWidget::on_comboBoxSerialNumber_currentIndexChanged( const QString & text ) {
    m_deviceSerialNumber = text;
    if (!text.isEmpty() && ui->comboBoxSerialNumber->isEnabled()) {
        m_ubuntuDeviceNotifier.startMonitoring(m_deviceSerialNumber);
        ui->lblDeviceInfo->setText(ui->comboBoxSerialNumber->itemData(ui->comboBoxSerialNumber->currentIndex()).toString());

        detectDeviceVersion();
    }
}

void UbuntuDevicesWidget::onDeviceConnected(QString serialNumber) {
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICES));
    if (settings.value(QLatin1String(Constants::SETTINGS_KEY_AUTOTOGGLE),Constants::SETTINGS_DEFAULT_DEVICES_AUTOTOGGLE).toBool()) {
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
    ui->lblLoading->show();
    ui->comboBoxSerialNumber->clear();
    detectDevices();
}


void UbuntuDevicesWidget::onError(QString msg) {
    ui->plainTextEdit->appendHtml(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONERROR).arg(msg));
}

void UbuntuDevicesWidget::beginAction(QString msg) {
    ui->plainTextEdit->appendHtml(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_BEGIN).arg(msg));
}

void UbuntuDevicesWidget::endAction(QString msg) {
    ui->plainTextEdit->appendHtml(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_END).arg(msg));
}


