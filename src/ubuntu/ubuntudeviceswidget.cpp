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
#include <QStandardPaths>
#include <QDesktopServices>

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
    ui->tabWidget_DeviceControlInput->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_DEVICE_CONTROL_SIMPLE_TAB);
    ui->stackedEmulatorConfigWidget->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_EMULATOR_PACKAGE_CHECK);

    m_instance = this;
    m_deviceDetected = false;
    m_aboutToClose = false;
    ui->progressBar_InstallEmulator->setMinimum(0);
    ui->progressBar_InstallEmulator->setMaximum(0);
    ui->progressBar_InstallEmulator->hide();
    ui->progressBar_CreateEmulator->setMinimum(0);
    ui->progressBar_CreateEmulator->setMaximum(0);
    ui->progressBar_CreateEmulator->hide();
    ui->label_InstallEmulatorQuestion->show();
    ui->pushButton_InstallEmulator_OK->show();
    ui->widgetSshProperties->hide();
    ui->pushButtonSshInstall->hide();
    ui->pushButtonSshRemove->hide();
    ui->widgetMovedToSettings->hide();
    
    ui->nameLineEdit->setInitialText( QLatin1String(Constants::UBUNTU_INITIAL_EMULATOR_NAME));
    slotChanged();
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

    connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));

    connect(ui->listWidget_EmulatorImages, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(startEmulator(QListWidgetItem*)));

    checkEmulator();
}


UbuntuDevicesWidget::~UbuntuDevicesWidget()
{
    m_aboutToClose = true;
    m_ubuntuProcess.stop();
    delete ui;
}

bool UbuntuDevicesWidget::validate() {
    if (!ui->nameLineEdit->isValid()) {
        ui->label_EmulatorValidationMessage->setText(QLatin1String(Constants::ERROR_MSG_EMULATOR_NAME));
        return false;
    }

    // Check existence of the directory
    QString projectDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    projectDir += QDir::separator();
    projectDir += QLatin1String(Constants::DEFAULT_EMULATOR_PATH);
    projectDir += QDir::separator();
    projectDir += ui->nameLineEdit->text();
    const QFileInfo projectDirFile(projectDir);
    if (!projectDirFile.exists()) { // All happy
        return true;
    }
    if (projectDirFile.isDir()) {
        ui->label_EmulatorValidationMessage->setText(QLatin1String(Constants::ERROR_MSG_EMULATOR_EXISTS));
        return false;
    }

    return true;
}

void UbuntuDevicesWidget::slotChanged()
{
    if (!validate()) {
        ui->pushButton_CreateNewEmulator->setEnabled(false);
    } else {
        ui->label_EmulatorValidationMessage->setText(QLatin1String(Constants::EMPTY));
        ui->pushButton_CreateNewEmulator->setEnabled(true);

    }
}


void UbuntuDevicesWidget::onMessage(QString msg) {
    if (msg.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_UNABLE_TO_FETCH))) {
        ui->progressBar_InstallEmulator->hide();
        ui->label_InstallEmulatorStatus->hide();
        ui->label_InstallEmulatorQuestion->show();
        ui->pushButton_InstallEmulator_OK->show();
    }
    m_reply.append(msg);
    ui->plainTextEdit->appendPlainText(msg.trimmed());
}

void UbuntuDevicesWidget::onStarted(QString cmd) {
    ui->stackedWidgetConnectedDevice->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_DEVICE_CONNECTIVITY_INPUT);
    ui->lblDeviceProcessInfo->setText(QFileInfo(cmd).baseName());
    ui->lblLoading->show();
}


void UbuntuDevicesWidget::onFinished(QString cmd, int code) {
    Q_UNUSED(code);

    ui->stackedWidgetConnectedDevice->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_DEVICE_CONNECTIVITY_INFO);
    if (m_aboutToClose) { return; }


    if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_LOCAL_START_EMULATOR).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        on_pushButtonRefresh_clicked();
    }

    if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_LOCAL_CREATE_EMULATOR).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        ui->progressBar_CreateEmulator->hide();
        ui->label_EmulatorValidationMessage->setText(QLatin1String(Constants::EMPTY));
        ui->pushButton_CreateNewEmulator->setEnabled(true);
        checkEmulatorInstances();
    }

    if (cmd ==  QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_LOCAL_SEARCH_IMAGES).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
        ui->listWidget_EmulatorImages->clear();
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.isEmpty()) {
                ui->pushButton_StartEmulator->setEnabled(false);
                continue;
            }
            ui->pushButton_StartEmulator->setEnabled(true);
            QListWidgetItem* item = new QListWidgetItem(line);
            ui->listWidget_EmulatorImages->addItem(item);
            ui->listWidget_EmulatorImages->setCurrentItem(item);
        }
        detectDevices();
    }
    if (cmd == QString::fromLatin1(Constants::UBUNTUWIDGETS_ONFINISHED_SCRIPT_LOCAL_PACKAGE_INSTALLED).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
        ui->stackedEmulatorConfigWidget->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_EMULATOR_PACKAGE_CHECK);
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
                ui->label_InstallEmulatorStatus->hide();
                ui->pushButton_InstallEmulator_OK->setEnabled(true);
                detectDevices();
            } else {
                QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                QString sEmulatorPackageStatus = lineData.takeFirst();
                QString sEmulatorPackageName = lineData.takeFirst();
                QString sEmulatorPackageVersion = lineData.takeFirst();
                if (sEmulatorPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
                    checkEmulatorInstances();
                    ui->label_EmulatorInfo->setText(QString::fromLatin1(Ubuntu::Constants::UBUNTUDEVICESWIDGET_LABEL_EMULATOR_INFO).arg(sEmulatorPackageVersion).arg(sEmulatorPackageName));
                    ui->stackedEmulatorConfigWidget->setCurrentIndex(Ubuntu::Constants::UBUNTUDEVICESWIDGET_PAGE_EMULATOR_INSTANCES);
                }
            }
        }
    }

    if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_LOCAL_INSTALL_EMULATOR).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_UNABLE_TO_FETCH))) {
                ui->progressBar_InstallEmulator->hide();
                ui->label_InstallEmulatorStatus->hide();
                ui->label_InstallEmulatorQuestion->show();
                ui->pushButton_InstallEmulator_OK->hide();
            }
        }
        checkEmulator();
    }


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
            m_deviceDetected = false;

            ui->stackedWidgetDeviceConnected->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_DEVICE_CONNECTIVITY_INFO);
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NO_DEVICE));
        } else if (lines.count() > 0) {
            ui->frameNoDevices->hide();
            ui->widgetDeviceSerial->show();
            m_deviceDetected = true;
            ui->stackedWidgetDeviceConnected->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_DEVICE_CONNECTIVITY_INPUT);
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

void UbuntuDevicesWidget::checkEmulatorInstances(){
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));

}

void UbuntuDevicesWidget::checkEmulator() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));
    m_ubuntuProcess.stop();
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));
}

void UbuntuDevicesWidget::on_pushButton_InstallEmulator_OK_clicked() {
    ui->progressBar_InstallEmulator->show();
    ui->label_InstallEmulatorStatus->show();
    ui->label_InstallEmulatorQuestion->hide();
    ui->pushButton_InstallEmulator_OK->hide();
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
}

void UbuntuDevicesWidget::on_pushButton_CreateNewEmulator_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));
    m_ubuntuProcess.stop();
    ui->progressBar_CreateEmulator->show();
    ui->label_EmulatorValidationMessage->setText(QLatin1String(Constants::MSG_EMULATOR_IS_CREATED));
    ui->pushButton_CreateNewEmulator->setEnabled(false);
    QString strEmulatorName = ui->nameLineEdit->text();
    QString strEmulatorPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    strEmulatorPath += QDir::separator();
    strEmulatorPath += QLatin1String(Constants::DEFAULT_EMULATOR_PATH);
    strEmulatorPath += QDir::separator();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(strEmulatorPath).arg(strEmulatorName) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));

}

void UbuntuDevicesWidget::on_pushButton_StartEmulator_clicked() {
    startEmulator(ui->listWidget_EmulatorImages->currentItem());
}

void UbuntuDevicesWidget::startEmulator(QListWidgetItem * item) {

    QStringList lineData = item->text().trimmed().split(QLatin1String(Constants::TAB));
    QString sEmulatorPath = lineData.takeFirst();
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR));
    m_ubuntuProcess.stop();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPath) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR));
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


