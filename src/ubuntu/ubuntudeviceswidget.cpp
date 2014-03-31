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
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <utils/qtcassert.h>
#include "ubuntuconstants.h"

#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

using namespace Ubuntu;

UbuntuDevicesWidget *UbuntuDevicesWidget::m_instance = 0;

UbuntuDevicesWidget *UbuntuDevicesWidget::instance()
{
    return m_instance;
}

bool UbuntuDevicesWidget::deviceDetected()
{
    return ui->comboBoxSerialNumber->count();
}

UbuntuDevicesWidget::UbuntuDevicesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UbuntuDevicesWidget)
    , m_ubuntuProcess(new Ubuntu::Internal::UbuntuProcess(this))
    , m_refreshKnownAfterScan(false)

{
    ui->setupUi(this);
    //Disable the clone network button for now
    ui->pushButtonCloneNetworkConfig->setVisible(false);

    ui->stackedEmulatorConfigWidget->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_EMULATOR_PACKAGE_CHECK);

    m_instance = this;
    m_aboutToClose = false;
    ui->progressBar_InstallEmulator->setMinimum(0);
    ui->progressBar_InstallEmulator->setMaximum(0);
    ui->progressBar_InstallEmulator->hide();
    ui->progressBar_CreateEmulator->setMinimum(0);
    ui->progressBar_CreateEmulator->setMaximum(0);
    ui->progressBar_CreateEmulator->hide();
    ui->label_InstallEmulatorQuestion->show();
    ui->pushButton_InstallEmulator_OK->show();
    
    ui->nameLineEdit->setInitialText( QLatin1String(Constants::UBUNTU_INITIAL_EMULATOR_NAME));
    slotChanged();

    //devices page
    setupDevicePage();

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);

    connect(m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString, int)));
    connect(m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));

    connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));

    connect(ui->listWidget_EmulatorImages, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(startEmulator(QListWidgetItem*)));

    connect(&m_ubuntuDeviceNotifier,SIGNAL(deviceConnected(QString)),this,SLOT(onDeviceConnected(QString)));

    ProjectExplorer::DeviceManager* devMgr = ProjectExplorer::DeviceManager::instance();
    connect(devMgr,SIGNAL(devicesLoaded()),this,SLOT(readDevicesFromSettings()));
    connect(devMgr,SIGNAL(deviceAdded(Core::Id)),this,SLOT(deviceAdded(Core::Id)));
    connect(devMgr,SIGNAL(deviceRemoved(Core::Id)),this,SLOT(deviceRemoved(Core::Id)));
    connect(devMgr,SIGNAL(deviceUpdated(Core::Id)),this,SLOT(deviceUpdated(Core::Id)));
}


UbuntuDevicesWidget::~UbuntuDevicesWidget()
{
    m_aboutToClose = true;
    m_ubuntuProcess->stop();
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
    ui->progressBar->show();
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
        //detectDevices();
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
                //detectDevices();
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

        //ugly hack to prevent running the search script and device
        //detection scripts at the same time
        if(m_refreshKnownAfterScan) {
            m_refreshKnownAfterScan = false;

            foreach(Ubuntu::Internal::UbuntuDevice::Ptr ptr, m_knownDevices) {
                removeDevice(ptr.data());
                ptr->helper()->refresh();
            }
        }

        // fill combobox data
        ui->comboBoxSerialNumber->setEnabled(false);
        foreach(QString line, lines) {
            line = line.trimmed();

            if (line.isEmpty()) {
                continue;
            }

            QRegularExpression exp(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_ADB_REGEX));
            QRegularExpressionMatch match = exp.match(line);

            if(match.hasMatch()) {
                QStringList lineData = match.capturedTexts();

                //The first entry is always the full match
                //which means in our case its the complete string
                lineData.takeFirst();

                if (lineData.count() == 2) {
                    QString sSerialNumber = lineData.takeFirst();
                    QString sDeviceInfo = lineData.takeFirst();

                    //sometimes the adb server is not started when adb devices is
                    //executed, we just skip those lines
                    if(sSerialNumber == QLatin1String("*"))
                        continue;

                    if(sSerialNumber.isEmpty() || sSerialNumber.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_ADB_NOACCESS))) {
                        continue;
                    }

                    registerNewDevice(sSerialNumber,sDeviceInfo);
                }
            }
        }

        // if there are no devices
        if (ui->comboBoxSerialNumber->count() == 0) {
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NO_DEVICE));
        } else if (lines.count() > 0) {
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_FOUND_DEVICES).arg(lines.count()));

            //detectDeviceVersion();
        }
        emit updateDeviceActions();

    } else if (cmd == QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_DEVICEVERSION).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVICEVERSION_DETECTED));
        //detectHasNetworkConnection();
    }
    else {
        // left empty
    }

    ui->lblLoading->hide();
    m_reply.clear();
}

/*!
 * \brief UbuntuDevicesWidget::readDevicesFromSettings
 * read all known devices from the DeviceManager, this is triggered
 * automatically on startup
 */
void UbuntuDevicesWidget::readDevicesFromSettings()
{
    ProjectExplorer::DeviceManager* devMgr = ProjectExplorer::DeviceManager::instance();
    for(int i = 0; i < devMgr->deviceCount(); i++) {
        ProjectExplorer::IDevice::ConstPtr dev = devMgr->deviceAt(i);
        if(dev && dev->type() == Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID)) {

            //ugly hack to get a mutable version of the device
            //no idea why its necessary to lock it up
            Ubuntu::Internal::UbuntuDevice* cPtr = qSharedPointerCast<const Ubuntu::Internal::UbuntuDevice>(dev)->helper()->device();
            if(cPtr) {
                m_knownDevices.insert(cPtr->id().uniqueIdentifier(),cPtr->sharedFromThis());
                connect(cPtr->helper(),SIGNAL(featureDetected()),this,SLOT(knownDeviceFeatureChange()));
                addDevice(cPtr);
            }
        }
    }

    setupDevicePage();

    //will trigger detectDevices
    checkEmulator();
}

/*!
 * \brief UbuntuDevicesWidget::deviceAdded
 * A device was added in the DeviceManager, check if know it and
 * if we should know it. If its a new Ubuntu device its added to
 * the known devices
 */
void UbuntuDevicesWidget::deviceAdded(const Core::Id &id)
{
    qDebug()<<"Device Manager reports device added: "<<id.toString();
    if(m_knownDevices.contains(id.uniqueIdentifier()))
        return;

    ProjectExplorer::IDevice::ConstPtr ptr = ProjectExplorer::DeviceManager::instance()->find(id);
    if(!ptr)
        return;

    if(ptr->type() != Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID))
        return;

    Ubuntu::Internal::UbuntuDevice::ConstPtr ubuntuDev
            = qSharedPointerCast<const Ubuntu::Internal::UbuntuDevice>(ptr);

    Ubuntu::Internal::UbuntuDeviceHelper* hlp = ubuntuDev->helper();
    Ubuntu::Internal::UbuntuDevice* dev = hlp->device();

    m_knownDevices.insert(dev->id().uniqueIdentifier(),dev->sharedFromThis());
    if(dev->deviceState() == ProjectExplorer::IDevice::DeviceConnected) {
        addDevice(dev);
    }

    connect(dev->helper(),SIGNAL(featureDetected()),this,SLOT(knownDeviceFeatureChange()));
}

/*!
 * \brief UbuntuDevicesWidget::deviceRemoved
 * A device was removed from the device manager, if its one of ours
 * we will also remove it
 */
void UbuntuDevicesWidget::deviceRemoved(const Core::Id &id)
{
    qDebug()<<"Device Manager reports device removed: "<<id.toString();
    if(!m_knownDevices.contains(id.uniqueIdentifier()))
        return;

    Ubuntu::Internal::UbuntuDevice::Ptr dev = m_knownDevices[id.uniqueIdentifier()];
    removeDevice(dev.data());
}

/*!
 * \brief UbuntuDevicesWidget::deviceUpdated
 * called when a device state is changed between connected
 * and disconnected, adds or removes the config widget
 * accordingly
 */
void UbuntuDevicesWidget::deviceUpdated(const Core::Id &id)
{
    if(!m_knownDevices.contains(id.uniqueIdentifier()))
        return;

    Ubuntu::Internal::UbuntuDevice::Ptr dev = m_knownDevices[id.uniqueIdentifier()];
    if(dev->deviceState() == ProjectExplorer::IDevice::DeviceConnected
            || dev->deviceState() == ProjectExplorer::IDevice::DeviceReadyToUse) {
        addDevice(dev.data());
    } else {
        removeDevice(dev.data());
    }
}

void UbuntuDevicesWidget::knownDeviceFeatureChange()
{
    Ubuntu::Internal::UbuntuDeviceHelper* hlpr = qobject_cast<Ubuntu::Internal::UbuntuDeviceHelper*>(sender());
    if(!hlpr) return;
    if(hlpr->device()->id().uniqueIdentifier() == ui->comboBoxSerialNumber->currentData().toInt())
        setupDevicePage();
}

void UbuntuDevicesWidget::checkEmulatorInstances(){
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));
    m_ubuntuProcess->stop();
    m_ubuntuProcess->append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH) << QApplication::applicationDirPath());
    m_ubuntuProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));

}

void UbuntuDevicesWidget::checkEmulator() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));
    m_ubuntuProcess->stop();
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_ubuntuProcess->append(QStringList() << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));
}

void UbuntuDevicesWidget::on_pushButton_InstallEmulator_OK_clicked() {
    ui->progressBar_InstallEmulator->show();
    ui->label_InstallEmulatorStatus->show();
    ui->label_InstallEmulatorQuestion->hide();
    ui->pushButton_InstallEmulator_OK->hide();
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_ubuntuProcess->stop();
    m_ubuntuProcess->append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
}

void UbuntuDevicesWidget::on_pushButton_CreateNewEmulator_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));
    m_ubuntuProcess->stop();
    ui->progressBar_CreateEmulator->show();
    ui->label_EmulatorValidationMessage->setText(QLatin1String(Constants::MSG_EMULATOR_IS_CREATED));
    ui->pushButton_CreateNewEmulator->setEnabled(false);
    QString strEmulatorName = ui->nameLineEdit->text();
    QString strEmulatorPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    strEmulatorPath += QDir::separator();
    strEmulatorPath += QLatin1String(Constants::DEFAULT_EMULATOR_PATH);
    strEmulatorPath += QDir::separator();
    m_ubuntuProcess->append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(strEmulatorPath).arg(strEmulatorName) << QApplication::applicationDirPath());
    m_ubuntuProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));

}

void UbuntuDevicesWidget::on_pushButton_StartEmulator_clicked() {
    startEmulator(ui->listWidget_EmulatorImages->currentItem());
}

void UbuntuDevicesWidget::startEmulator(QListWidgetItem * item) {

    QStringList lineData = item->text().trimmed().split(QLatin1String(Constants::TAB));
    QString sEmulatorPath = lineData.takeFirst();
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR));
    m_ubuntuProcess->stop();
    m_ubuntuProcess->append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPath) << QApplication::applicationDirPath());
    m_ubuntuProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR));
}

void UbuntuDevicesWidget::setupDevicePage()
{
    if(ui->comboBoxSerialNumber->count() == 0) {
        ui->frameNoDevices->show();
        ui->widgetDeviceSerial->hide();
        ui->comboBoxSerialNumber->clear();
        ui->comboBoxSerialNumber->setEnabled(false);

        ui->stackedWidgetDeviceConnected->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_DEVICE_CONNECTIVITY_NODEVICE_CONNECTED);
    } else {
        ui->stackedWidgetDeviceConnected->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_PAGE_DEVICE_CONNECTIVITY_DEVICE_CONNECTED);
        ui->frameNoDevices->hide();
        ui->widgetDeviceSerial->show();
        ui->comboBoxSerialNumber->setEnabled(true);

        int devId = ui->comboBoxSerialNumber->currentData().toInt();
        if(m_knownDevices.contains(devId)) {
            Ubuntu::Internal::UbuntuDevice::Ptr dev = m_knownDevices[devId];
            if(dev->hasNetworkConnection() != Ubuntu::Internal::UbuntuDevice::Available) {
                ui->stackedWidgetDeveloperMode->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_DEVELOPERMODE_PAGE_NONETWORK);
            } else {
                if(dev->developerModeEnabled() == Ubuntu::Internal::UbuntuDevice::Available) {
                    ui->stackedWidgetDeveloperMode->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_DEVELOPERMODE_PAGE_ENABLED);
                } else {
                    ui->stackedWidgetDeveloperMode->setCurrentIndex(Constants::UBUNTUDEVICESWIDGET_DEVELOPERMODE_PAGE_DEVICEFOUND);
                }
            }
        }
    }
}

void UbuntuDevicesWidget::detectDevices() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES));
    m_ubuntuProcess->stop();
    m_ubuntuProcess->append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH) << QApplication::applicationDirPath());
    m_ubuntuProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES));
}

#if 0
//this was not used, still needed?
void UbuntuDevicesWidget::on_pushButtonCloneNetworkConfig_clicked() {
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK));
    m_ubuntuProcess->stop();
    m_ubuntuProcess->append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(serialNumber()) << QApplication::applicationDirPath());
    m_ubuntuProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK));
}
#endif


QString UbuntuDevicesWidget::serialNumber() {
    return ui->comboBoxSerialNumber->currentText();
}

void UbuntuDevicesWidget::on_comboBoxSerialNumber_currentIndexChanged( const QString & text )
{
    if(text.isEmpty())
        return;

    int idx = ui->comboBoxSerialNumber->findText(text);
    Q_ASSERT_X(idx >= 0, Q_FUNC_INFO, "Index can not be invalid");

    int devId = ui->comboBoxSerialNumber->itemData(idx).toInt();

    ui->stackedWidgetDeviceConfig->setCurrentIndex(idx);
    ui->lblDeviceInfo->setText(m_knownDevices[devId]->deviceInfoString());

    setupDevicePage();
}

void UbuntuDevicesWidget::onDeviceConnected(const QString &id) {
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICES));
    if (settings.value(QLatin1String(Constants::SETTINGS_KEY_AUTOTOGGLE),Constants::SETTINGS_DEFAULT_DEVICES_AUTOTOGGLE).toBool()) {
        Core::ModeManager::activateMode(Ubuntu::Constants::UBUNTU_MODE_DEVICES);
    }

    Core::Id qtcid = Core::Id::fromSetting(id);
    if(m_knownDevices.contains(qtcid.uniqueIdentifier()))
        return;

    registerNewDevice(id,QString());
    //this would normally trigger device detection, which kills adb
    //but we already know a new device is connected so there is no
    //need to mess with adb
#if 0
    m_reply.clear();

    ui->plainTextEdit->clear();

    m_ubuntuProcess->stop();

    ui->frameNoDevices->hide();
    ui->frameProgress->show();
    ui->lblLoading->show();
    detectDevices();
#endif
}


void UbuntuDevicesWidget::on_pushButtonRefresh_clicked() {
    m_refreshKnownAfterScan = true;
    m_ubuntuProcess->stop();
    ui->plainTextEdit->clear();
    m_reply.clear();
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

/*!
 * \brief UbuntuDevicesWidget::addDevice
 * Adds a device to the dropdown box, and creates the config widget
 * for it
 */
int UbuntuDevicesWidget::addDevice(Internal::UbuntuDevice *dev)
{
    int idx = ui->comboBoxSerialNumber->findData(dev->id().uniqueIdentifier());

    //already there
    if(idx >= 0)
        return idx;

    qDebug()<<"Show device "<<dev->id().toString()<<" Because its in connected state";

    if(dev->deviceState() == ProjectExplorer::IDevice::DeviceConnected
            || dev->deviceState() == ProjectExplorer::IDevice::DeviceReadyToUse) {
        ui->comboBoxSerialNumber->addItem(dev->id().toSetting().toString(),dev->id().uniqueIdentifier());

        //pointer ownership goes to the stack
        idx = ui->stackedWidgetDeviceConfig->addWidget(dev->createWidget());
    }

    setupDevicePage();
    return idx;
}

/*!
 * \brief UbuntuDevicesWidget::removeDevice
 * Removes a device from the dropdown box and deletes the config widget
 */
void UbuntuDevicesWidget::removeDevice(Internal::UbuntuDevice *dev)
{
    int idx = ui->comboBoxSerialNumber->findData(dev->id().uniqueIdentifier());
    if(idx < 0)
        return;

    QWidget* w = ui->stackedWidgetDeviceConfig->widget(idx);
    ui->stackedWidgetDeviceConfig->removeWidget(w);
    delete w;

    ui->comboBoxSerialNumber->removeItem(idx);

    setupDevicePage();
}

/*!
 * \brief UbuntuDevicesWidget::registerNewDevice
 * Registers a new device in the device manager if its not
 * already in the known devices map.
 * \note will not add it into the devices dropdown box, this
 *       will happen automatically when the device switches to
 *       connected state
 */
void UbuntuDevicesWidget::registerNewDevice(const QString &serial, const QString &deviceInfo)
{
    if(!m_knownDevices.contains(Core::Id::fromSetting(serial).uniqueIdentifier())) {

        bool isEmu = serial.startsWith(QLatin1String("emulator"));

        Ubuntu::Internal::UbuntuDevice::Ptr dev = Ubuntu::Internal::UbuntuDevice::create(
                    tr("Ubuntu Device")
                    , serial
                    , isEmu ? ProjectExplorer::IDevice::Emulator : ProjectExplorer::IDevice::Hardware
                    , ProjectExplorer::IDevice::AutoDetected);


        dev->setDeviceInfoString(deviceInfo);
        ProjectExplorer::DeviceManager::instance()->addDevice(dev);
    }
}
