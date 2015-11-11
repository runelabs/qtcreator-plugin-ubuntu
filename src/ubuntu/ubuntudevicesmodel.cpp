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

#include "ubuntudevicesmodel.h"
#include "ubuntuconstants.h"
#include "ubuntukitmanager.h"
#include "ubuntuscopefinalizer.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <utils/projectintropage.h>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QVariant>
#include <QMutableStringListIterator>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QTimer>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuDevicesModel::UbuntuDevicesModel(QObject *parent) :
    QAbstractListModel(parent),
    m_cancellable(false),
    m_state(Initial)
{
    m_deviceNotifier = new UbuntuDeviceNotifier(this);
    connect(m_deviceNotifier,SIGNAL(deviceConnected(QString)),this,SLOT(deviceConnected(QString)));

    m_process = new UbuntuProcess(this);
    connect(m_process,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(m_process,SIGNAL(finished(QString,int)),this,SLOT(processFinished(QString,int)));
    connect(m_process,SIGNAL(stdOut(QString)),this,SIGNAL(stdOutMessage(QString)));
    connect(m_process,SIGNAL(error(QString)),this,SIGNAL(stdErrMessage(QString)));


    ProjectExplorer::KitManager* devMgr = static_cast<ProjectExplorer::KitManager*>(ProjectExplorer::KitManager::instance());
    connect(devMgr,SIGNAL(kitsLoaded()),this,SLOT(refresh()));
    connect(ProjectExplorer::DeviceManager::instance(),SIGNAL(deviceListReplaced()),this,SLOT(readDevicesFromSettings()));
}

bool UbuntuDevicesModel::set(int index, const QString &role, const QVariant &value)
{
    if(index < 0 || index >= rowCount())
        return false;

    QModelIndex idx = createIndex(index,0);
    if(!roleNames().values().contains(role.toUtf8()))
        return false;

    return setData(idx,value,roleNames().key(role.toUtf8()));
}

int UbuntuDevicesModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;
    return m_knownDevices.size();
}

bool UbuntuDevicesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(debug) qDebug()<<"Setting index "<<index<<" with data "<<value<<" to role "<<role;

    if(!index.isValid()
            || index.parent().isValid()
            || index.row() < 0
            || index.row() > rowCount())
        return false;

    UbuntuDevice::Ptr dev = m_knownDevices[index.row()]->device();

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            dev->setDisplayName(value.toString());
            emit dataChanged(index,index,QVector<int>()<<Qt::DisplayRole<<Qt::EditRole);
            return false;
        case KitListRole:
            return false;
        case DeveloperModeRole: {
            if(value.type() != QVariant::Bool)
                return false;

            bool set = value.toBool();
            UbuntuDevice::FeatureState newState = set ? UbuntuDevice::Available : UbuntuDevice::NotAvailable;
            UbuntuDevice::FeatureState oldState = dev->developerModeEnabled();

            if(oldState == UbuntuDevice::Unknown)
                return false;

            if( oldState != newState )
                dev->setDeveloperModeEnabled(set);
            return true;
            break;
        }
        case NetworkConnectionRole: {
            if(value.type() != QVariant::Bool)
                return false;

            bool set = value.toBool();
            UbuntuDevice::FeatureState oldState = dev->hasNetworkConnection();

            if( oldState == UbuntuDevice::Unknown || oldState == UbuntuDevice::Available )
                return false;

            if(set)
                dev->cloneNetwork();
            return true;

            break;
        }
        case WriteableImageRole: {
            if(value.type() != QVariant::Bool)
                return false;

            bool set = value.toBool();
            UbuntuDevice::FeatureState newState = set ? UbuntuDevice::Available : UbuntuDevice::NotAvailable;
            UbuntuDevice::FeatureState oldState = dev->hasWriteableImage();

            if(oldState == UbuntuDevice::Unknown)
                return false;

            if( oldState != newState )
                dev->setWriteableImageEnabled(set);
            return true;
            break;
        }
        case DeveloperToolsRole: {
            if(value.type() != QVariant::Bool)
                return false;

            bool set = value.toBool();
            UbuntuDevice::FeatureState newState = set ? UbuntuDevice::Available : UbuntuDevice::NotAvailable;
            UbuntuDevice::FeatureState oldState = dev->hasDeveloperTools();

            if(oldState == UbuntuDevice::Unknown)
                return false;

            if( oldState != newState )
                dev->setDeveloperToolsInstalled(set);
            return true;
            break;
        }
        case EmulatorScaleFactorRole: {
            if(value.type() != QVariant::String)
                return false;

            QString set = value.toString();
            if(dev->scaleFactor() != set)
                return dev->setScaleFactor(set);

            return true;
            break;
        }
        case EmulatorMemorySettingRole: {
            if(value.type() != QVariant::String)
                return false;

            QString set = value.toString();
            if(dev->memorySetting() != set)
                return dev->setMemorySetting(set);

            return true;
            break;
        }
        default:
            break;
    }

    return false;
}

QVariant UbuntuDevicesModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()
            || index.parent().isValid()
            || index.row() < 0
            || index.row() > rowCount())
        return QVariant();

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return m_knownDevices[index.row()]->device()->displayName();
        case UniqueIdRole:
            return m_knownDevices[index.row()]->id().uniqueIdentifier();
        case DetectionStateRole:
            return m_knownDevices[index.row()]->device()->detectionState();
        case DetectionStateStringRole:
            return m_knownDevices[index.row()]->device()->detectionStateString();
        case ConnectionStateRole:
            return m_knownDevices[index.row()]->device()->deviceState();
        case ConnectionStateStringRole:
            return m_knownDevices[index.row()]->device()->deviceStateToString();
        case KitListRole:
            return QVariant::fromValue(m_knownDevices[index.row()]->kits());
        case DeveloperModeRole:
            return m_knownDevices[index.row()]->device()->developerModeEnabled();
        case NetworkConnectionRole:
            return m_knownDevices[index.row()]->device()->hasNetworkConnection();
        case WriteableImageRole:
            return m_knownDevices[index.row()]->device()->hasWriteableImage();
        case DeveloperToolsRole:
            return m_knownDevices[index.row()]->device()->hasDeveloperTools();
        case LogRole:
            return m_knownDevices[index.row()]->device()->helper()->log();
        case SerialIdRole:
            return m_knownDevices[index.row()]->device()->serialNumber();
        case ModelInfoRole:
            return m_knownDevices[index.row()]->device()->modelInfo();
        case DeviceInfoRole:
            return m_knownDevices[index.row()]->device()->deviceInfo();
        case ProductInfoRole:
            return m_knownDevices[index.row()]->device()->productInfo();
        case MachineTypeRole:
            return m_knownDevices[index.row()]->device()->machineType();
        case FrameworkRole:
            return m_knownDevices[index.row()]->device()->framework();
        case EmulatorImageRole:
            return m_knownDevices[index.row()]->device()->imageName();
        case EmulatorDeviceVersionRole:
            return m_knownDevices[index.row()]->device()->deviceVersion();
        case EmulatorUbuntuVersionRole:
            return m_knownDevices[index.row()]->device()->ubuntuVersion();
        case EmulatorImageVersionRole:
            return m_knownDevices[index.row()]->device()->imageVersion();
        case EmulatorScaleFactorRole:
            return m_knownDevices[index.row()]->device()->scaleFactor();
        case EmulatorMemorySettingRole:
            return m_knownDevices[index.row()]->device()->memorySetting();
        default:
            break;
    }
    return QVariant();
}

QHash<int, QByteArray> UbuntuDevicesModel::roleNames() const
{
    QHash<int,QByteArray> roles = QAbstractListModel::roleNames();
    roles.insert(UniqueIdRole,"deviceId");
    roles.insert(DetectionStateRole,"detectionState");
    roles.insert(DetectionStateStringRole,"detectionStateString");
    roles.insert(ConnectionStateRole,"connectionState");
    roles.insert(ConnectionStateStringRole,"connectionStateString");
    roles.insert(KitListRole,"kits");
    roles.insert(DeveloperModeRole,"developerModeEnabled");
    roles.insert(NetworkConnectionRole,"hasNetworkConnection");
    roles.insert(WriteableImageRole,"hasWriteableImage");
    roles.insert(DeveloperToolsRole,"hasDeveloperTools");
    roles.insert(LogRole,"deviceLog");
    roles.insert(SerialIdRole,"serial");
    roles.insert(ModelInfoRole,"modelInfo");
    roles.insert(DeviceInfoRole,"deviceInfo");
    roles.insert(ProductInfoRole,"productInfo");
    roles.insert(MachineTypeRole,"machineType");
    roles.insert(FrameworkRole,"frameworkVersion");
    roles.insert(EmulatorImageRole,"emulatorImageName");
    roles.insert(EmulatorDeviceVersionRole,"emuDeviceVersion");
    roles.insert(EmulatorUbuntuVersionRole,"emuUbuntuVersion");
    roles.insert(EmulatorImageVersionRole,"emuImageVersion");
    roles.insert(EmulatorScaleFactorRole,"emulatorScaleFactor");
    roles.insert(EmulatorMemorySettingRole,"emulatorMemorySetting");
    return roles;
}

Qt::ItemFlags UbuntuDevicesModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

void UbuntuDevicesModel::triggerPortForwarding(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->enablePortForward();
}

void UbuntuDevicesModel::triggerSSHSetup(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->deployPublicKey();
}

void UbuntuDevicesModel::triggerSSHConnection(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->openTerminal();
}

void UbuntuDevicesModel::triggerKitAutocreate(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    UbuntuKitManager::autoCreateKit(m_knownDevices[row]->device());
}

void UbuntuDevicesModel::triggerKitRemove(const int devId, const QVariant &kitid)
{
    int row = findDevice(devId);
    if(row < 0)
        return;

    ProjectExplorer::Kit* k = ProjectExplorer::KitManager::find(Core::Id::fromSetting(kitid));
    if(ProjectExplorer::DeviceKitInformation::deviceId(k) == Core::Id::fromUniqueIdentifier(devId)) {
        //completely delete the kit
        ProjectExplorer::KitManager::deregisterKit(k);
    }
}

void UbuntuDevicesModel::triggerRedetect(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->helper()->refresh();
}

void UbuntuDevicesModel::deleteDevice(const int devId)
{
    int index = findDevice(devId);
    if(index < 0) {
        QMessageBox::critical(Core::ICore::mainWindow(),tr("Could not delete device"),tr("The device ID is unknown, please press the refresh button and try again."));
        return;
    }

    ProjectExplorer::DeviceManager::instance()->removeDevice(m_knownDevices[index]->device()->id());
}

void UbuntuDevicesModel::refresh()
{
    readDevicesFromSettings();
    checkEmulatorInstalled();
}

void UbuntuDevicesModel::clear()
{
    if(rowCount()) {
        beginRemoveRows(QModelIndex(),0,rowCount()-1);
        qDeleteAll(m_knownDevices.begin(),m_knownDevices.end());
        m_knownDevices.clear();
        endRemoveRows();
    }
}

int UbuntuDevicesModel::findDevice(int uniqueIdentifier) const
{
    for ( int i = 0; i < m_knownDevices.size(); i++ ) {
        if(m_knownDevices[i]->id().uniqueIdentifier() == uniqueIdentifier)
            return i;
    }
    return -1;
}

bool UbuntuDevicesModel::hasDevice(int uniqueIdentifier) const
{
    return findDevice(uniqueIdentifier) >= 0;
}

UbuntuDevice::ConstPtr UbuntuDevicesModel::device(const int index)
{
    if(index < 0 || index >= rowCount())
        return UbuntuDevice::ConstPtr();
    return m_knownDevices[index]->device();
}

UbuntuDevicesItem *UbuntuDevicesModel::createItem(UbuntuDevice::Ptr dev)
{
    UbuntuDevicesItem *devItem = new UbuntuDevicesItem(dev,this);
    connect(devItem,SIGNAL(kitsChanged()),this,SLOT(kitsChanged()));
    connect(devItem,SIGNAL(detectionStateChanged()),this,SLOT(detectionStateChanged()));
    connect(devItem,SIGNAL(featureDetected()),this,SLOT(featureDetected()));
    connect(devItem,SIGNAL(logUpdated()),this,SLOT(logUpdated()));
    connect(devItem,SIGNAL(deviceInfoUpdated()),this,SLOT(deviceInfoUpdated()));
    return devItem;
}

/*!
 * \brief UbuntuDevicesModel::indexFromHelper
 * Checks if the passed QObject is a DeviceHelper and returns the
 * row index of the related device
 */
int UbuntuDevicesModel::indexFromHelper(QObject *possibleHelper)
{
    UbuntuDevicesItem* hlpr = qobject_cast<UbuntuDevicesItem*>(possibleHelper);
    if(!hlpr) return -1;
    return findDevice(hlpr->id().uniqueIdentifier());
}

void UbuntuDevicesModel::deviceChanged(QObject *possibleHelper, const QVector<int> &relatedRoles)
{
    int idx = indexFromHelper(possibleHelper);
    if(idx < 0)
        return;
    QModelIndex changed = createIndex(idx,0);
    emit dataChanged(changed,changed,relatedRoles);
}

/*!
 * \brief UbuntuDevicesModel::readDevicesFromSettings
 * read all known devices from the DeviceManager, this is triggered
 * automatically on startup
 */
void UbuntuDevicesModel::readDevicesFromSettings()
{
    clear();

    QList<UbuntuDevicesItem*> devs;
    ProjectExplorer::DeviceManager* devMgr = ProjectExplorer::DeviceManager::instance();
    for(int i = 0; i < devMgr->deviceCount(); i++) {
        ProjectExplorer::IDevice::ConstPtr dev = devMgr->deviceAt(i);
        if(dev && dev->type().toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID))) {

            //ugly hack to get a mutable version of the device
            //no idea why its necessary to lock it up
            Ubuntu::Internal::UbuntuDevice* cPtr = qSharedPointerCast<const Ubuntu::Internal::UbuntuDevice>(dev)->helper()->device();
            if(cPtr)
                devs.append(createItem(cPtr->sharedFromThis()));

        }
    }

    if (devs.count()) {
        beginInsertRows(QModelIndex(),0,devs.count()-1);
        m_knownDevices = devs;
        endInsertRows();
    }

    connect(devMgr,SIGNAL(deviceAdded(Core::Id)),this,SLOT(deviceAdded(Core::Id)),Qt::UniqueConnection);
    connect(devMgr,SIGNAL(deviceRemoved(Core::Id)),this,SLOT(deviceRemoved(Core::Id)),Qt::UniqueConnection);
    connect(devMgr,SIGNAL(deviceUpdated(Core::Id)),this,SLOT(deviceUpdated(Core::Id)),Qt::UniqueConnection);
}

void UbuntuDevicesModel::detectionStateChanged()
{
    //contains the possible changed roles when this slot is called
    static QVector<int> relatedRoles = QVector<int>()
            << DetectionStateRole << DetectionStateStringRole;

    deviceChanged(sender(),relatedRoles);
}

void UbuntuDevicesModel::featureDetected()
{
    //contains the possible changed roles when this slot is called
    static QVector<int> relatedRoles = QVector<int>()
            << DeveloperModeRole  << NetworkConnectionRole
            << WriteableImageRole << DeveloperToolsRole;

    deviceChanged(sender(),relatedRoles);
}

void UbuntuDevicesModel::deviceInfoUpdated()
{
    //contains the possible changed roles when this slot is called
    static QVector<int> relatedRoles = QVector<int>()
            << SerialIdRole << ModelInfoRole
            << DeviceInfoRole << ProductInfoRole
            << EmulatorDeviceVersionRole << EmulatorUbuntuVersionRole
            << EmulatorImageVersionRole << EmulatorScaleFactorRole
            << EmulatorMemorySettingRole << FrameworkRole;

    deviceChanged(sender(),relatedRoles);
}

void UbuntuDevicesModel::logUpdated()
{
    //contains the possible changed roles when this slot is called
    static QVector<int> relatedRoles = QVector<int>()
            << LogRole;

    deviceChanged(sender(),relatedRoles);
}

void UbuntuDevicesModel::kitsChanged()
{
    //contains the possible changed roles when this slot is called
    static QVector<int> relatedRoles = QVector<int>()
            << KitListRole;

    deviceChanged(sender(),relatedRoles);
}

/*!
 * \brief UbuntuDevicesModel::deviceAdded
 * A device was added in the DeviceManager, check if know it and
 * if we should know it. If its a new Ubuntu device its added to
 * the known devices
 */
void UbuntuDevicesModel::deviceAdded(const Core::Id &id)
{
    ProjectExplorer::IDevice::ConstPtr ptr = ProjectExplorer::DeviceManager::instance()->find(id);
    if(!ptr)
        return;

    if(!ptr->type().toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID)))
        return;

    if(debug) qDebug()<<"Device Manager reports device added: "<<id.toString();

    if (hasDevice(id.uniqueIdentifier()))
        return;

    Ubuntu::Internal::UbuntuDevice::ConstPtr ubuntuDev
            = qSharedPointerCast<const Ubuntu::Internal::UbuntuDevice>(ptr);

    Ubuntu::Internal::UbuntuDeviceHelper* hlp = ubuntuDev->helper();
    Ubuntu::Internal::UbuntuDevice* dev = hlp->device();

    beginInsertRows(QModelIndex(),rowCount(),rowCount());
    m_knownDevices.append(createItem(dev->sharedFromThis()));
    endInsertRows();
}

/*!
 * \brief UbuntuDevicesWidget::deviceRemoved
 * A device was removed from the device manager, if its one of ours
 * we will also remove it
 */
void UbuntuDevicesModel::deviceRemoved(const Core::Id &id)
{
    int index = findDevice(id.uniqueIdentifier());
    if(index < 0)
        return;

    if(debug) qDebug()<<"Device Manager reports device removed: "<<id.toString();

    beginRemoveRows(QModelIndex(),index,index);
    delete m_knownDevices.takeAt(index);
    endRemoveRows();
}

/*!
 * \brief UbuntuDevicesModel::deviceUpdated
 * called when a device state is changed between connected
 * and disconnected or device data was changed
 */
void UbuntuDevicesModel::deviceUpdated(const Core::Id &id)
{
    //contains the possible changed roles when this slot is called
    static QVector<int> relatedRoles = QVector<int>()
            << Qt::DisplayRole << Qt::EditRole
            << ConnectionStateRole << ConnectionStateStringRole;

    int index = findDevice(id.uniqueIdentifier());
    if(index >= 0) {
        QModelIndex changed = createIndex(index,0);
        emit dataChanged(changed, changed,relatedRoles);
    }
}

void UbuntuDevicesModel::deviceConnected(const QString &id)
{
    int idx = findDevice(Core::Id::fromSetting(id).uniqueIdentifier());
    if(idx >= 0)
        return;

    refresh();
}

/*!
 * \brief UbuntuDevicesModel::registerNewDevice
 * Registers a new device in the device manager if its not
 * already in the known devices map.
 * \note will not add it into model list, this
 *       will happen automatically when the device is
 *       registered in the device manager
 */
void UbuntuDevicesModel::registerNewDevice(const QString &serial, const QString &arch)
{
    if(findDevice(Core::Id::fromSetting(serial).uniqueIdentifier()) >= 0)
        return;

    if(!ClickToolChain::supportedArchitectures().contains(arch)) {
        emit logMessage(tr("Error: Can not register device %1. Architecture %2 is not supported.")
                        .arg(serial)
                        .arg(arch));
        return;
    }

    Ubuntu::Internal::UbuntuDevice::Ptr dev = Ubuntu::Internal::UbuntuDevice::create(
                tr("Ubuntu Device")
                , serial
                , ProjectExplorer::IDevice::Hardware
                , arch
                , ProjectExplorer::IDevice::AutoDetected);

    ProjectExplorer::DeviceManager::instance()->addDevice(dev);
}

bool UbuntuDevicesModel::emulatorInstalled() const
{
    return m_emulatorInstalled;
}

void UbuntuDevicesModel::setEmulatorInstalled(bool arg)
{
    if (m_emulatorInstalled != arg) {
        m_emulatorInstalled = arg;
        emit emulatorInstalledChanged(arg);
    }
}

bool UbuntuDevicesModel::busy() const
{
    return m_busy;
}

void UbuntuDevicesModel::setBusy(bool arg)
{
    if (m_busy != arg) {
        m_busy = arg;
        emit busyChanged(arg);
    }
}

QString UbuntuDevicesModel::state() const
{
    switch(m_state) {
        case CheckEmulatorInstalled: {
            return tr("Checking if emulator tool is installed");
            break;
        }
        case InstallEmulator: {
            return tr("Installing emulator tool");
            break;
        }
        case CreateEmulatorImage: {
            return tr("Creating emulator image");
            break;
        }
        case ReadFromSettings:{
            return tr("Reading settings");
        }
        case FindImages:{
            return tr("Searching for emulator images");
            break;
        }
        case AdbList:{
            return tr("Querying ADB");
            break;
        }
        default:
            return QString();
            break;
    }
}

void UbuntuDevicesModel::setState(UbuntuDevicesModel::State newState)
{
    if(m_state != newState) {
        m_state = newState;
        if(m_state == UbuntuDevicesModel::Initial || m_state == UbuntuDevicesModel::Idle)
            setBusy(false);
        else
            setBusy(true);

        emit stateChanged(state());
    }
}

bool UbuntuDevicesModel::cancellable() const
{
    return m_cancellable;
}

void UbuntuDevicesModel::setCancellable(bool arg)
{
    if (m_cancellable != arg) {
        m_cancellable = arg;
        emit cancellableChanged(arg);
    }
}

void UbuntuDevicesModel::beginAction(const QString &msg)
{
    emit logMessage(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_BEGIN).arg(msg));
}

void UbuntuDevicesModel::endAction(const QString &msg)
{
    emit logMessage(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_END).arg(msg));
}

void UbuntuDevicesModel::checkEmulatorInstalled()
{
    setState(CheckEmulatorInstalled);
    setCancellable(false);
    m_emulatorInstalled = false;

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));
    m_process->stop();
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName)
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));

}

void UbuntuDevicesModel::findEmulatorImages()
{
    setState(FindImages);
    setCancellable(false);

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));
    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));
}

void UbuntuDevicesModel::installEmulator()
{
    if(m_emulatorInstalled)
        return;

    setState(InstallEmulator);
    setCancellable(false);

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName)
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
}

void UbuntuDevicesModel::doCreateEmulatorImage(UbuntuProcess *process, const QString &name, const QString &arch, const QString &channel, const QString &passwd)
{
    process->stop();
    QString strEmulatorName = name;
    QString strEmulatorPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    strEmulatorPath += QDir::separator();
    strEmulatorPath += QLatin1String(Constants::DEFAULT_EMULATOR_PATH);
    strEmulatorPath += QDir::separator();
    QString strUserName = QProcessEnvironment::systemEnvironment().value(QLatin1String("USER"));
    QString strUserHome = QProcessEnvironment::systemEnvironment().value(QLatin1String("HOME"));
    process->append(QStringList() << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR_SCRIPT)
                    .arg(QString::fromLatin1(Ubuntu::Constants::UBUNTU_SUDO_BINARY))
                    .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                    .arg(strEmulatorPath)
                    .arg(strEmulatorName)
                    .arg(arch)
                    .arg(channel)
                    .arg(passwd)
                    .arg(strUserName)
                    .arg(strUserHome)
                    << QCoreApplication::applicationDirPath());
    process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));
}

void UbuntuDevicesModel::createEmulatorImage(const QString &name, const QString &arch, const QString &channel, const QString &passwd)
{
    setState(CreateEmulatorImage);

    //@BUG this should be cancellable but the QProcess::kill call just blocks for a long time, and then returns
    //     with the process still running. Most likely because the subprocess is executed with pkexec and has
    //     elevated priviledges
    setCancellable(false);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));
    doCreateEmulatorImage(m_process,name,arch,channel,passwd);
}

void UbuntuDevicesModel::queryAdb()
{
    setState(AdbList);
    setCancellable(false);

    m_process->stop();

    bool restartAdb = true;

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES));
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(restartAdb ? QString::number(1) : QString::number(0))
                      << QCoreApplication::applicationDirPath());

    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES));
}

void UbuntuDevicesModel::startEmulator(const QString &name)
{
    int idx = findDevice(Core::Id::fromSetting(name).uniqueIdentifier());
    if(idx < 0)
        return;

    if(m_knownDevices[idx]->device()->startEmulator()) {
        //trigger the device detection
        m_knownDevices[idx]->device()->helper()->refresh();
    }
}

void UbuntuDevicesModel::stopEmulator(const QString &name)
{
    int idx = findDevice(Core::Id::fromSetting(name).uniqueIdentifier());
    if(idx < 0)
        return;

    QStringList args = QStringList() << name;
    if(QProcess::startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_STOP_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                               ,args
                               ,QCoreApplication::applicationDirPath())) {
    }
}

void UbuntuDevicesModel::deleteEmulator(const QString &name)
{
    int index = findDevice(Core::Id::fromSetting(name).uniqueIdentifier());
    if(index < 0)
        return;

    QStringList args = QStringList() << name;
    QProcess proc;
    proc.setWorkingDirectory(QCoreApplication::applicationDirPath());
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_DELETE_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH),
               args);
    proc.waitForFinished();

    if(proc.exitCode() == 0)
        ProjectExplorer::DeviceManager::instance()->removeDevice(m_knownDevices[index]->device()->id());
    else {
        emit logMessage(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONERROR).arg(QLatin1String(proc.readAll())));
        QMessageBox::critical(Core::ICore::mainWindow(),tr("Could not delete emulator"),tr("The emulator %1 could not be deleted because of a error, check the logs for details").arg(name));
    }

}

QVariant UbuntuDevicesModel::validateEmulatorName(const QString &name)
{
    QString error;
    bool result = Utils::ProjectIntroPage::validateProjectName(name,&error);

    if(result) {
        foreach (UbuntuDevicesItem *item, m_knownDevices) {
            if (item->device()->machineType() == ProjectExplorer::IDevice::Emulator) {
                if(item->device()->imageName() == name) {
                    result = false;
                    error  = tr("Emulator name already exists");
                    break;
                }
            }
        }
    }

    QVariantMap m;
    m.insert(QStringLiteral("valid"),result);
    m.insert(QStringLiteral("error"),error);
    return QVariant::fromValue(m);
}

void UbuntuDevicesModel::cancel()
{
    if (m_cancellable && m_state == CreateEmulatorImage) {
        m_process->stop();
    }
}

void UbuntuDevicesModel::onMessage(const QString &msg)
{
    if (msg.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_UNABLE_TO_FETCH))) {
        setEmulatorInstalled(false);
    }
    m_reply.append(msg);
}

void UbuntuDevicesModel::processFinished(const QString &, int exitCode)
{
    State lastState = m_state;
    setCancellable(false);

    OnScopeExit {
        if(m_state == lastState)
            setState(Idle);
    };

    switch(lastState) {
        case CheckEmulatorInstalled: {
            QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
            foreach(QString line, lines) {
                line = line.trimmed();
                if (line.isEmpty()) {
                    continue;
                }
                if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
                    setEmulatorInstalled(false);
                    queryAdb();
                } else {
                    QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                    QString sEmulatorPackageStatus = lineData.takeFirst();
                    //QString sEmulatorPackageName = lineData.takeFirst();
                    //QString sEmulatorPackageVersion = lineData.takeFirst();
                    if (sEmulatorPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
                        setEmulatorInstalled(true);
                        findEmulatorImages();
                    }
                }
            }
            break;
        }
        case InstallEmulator: {
            QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
            foreach(QString line, lines) {
                line = line.trimmed();
                if (line.isEmpty()) {
                    continue;
                }
                if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
                    setEmulatorInstalled(false);
                    queryAdb();
                    break;
                } else {
                    QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                    QString sEmulatorPackageStatus = lineData.takeFirst();
                    //QString sEmulatorPackageName = lineData.takeFirst();
                    //QString sEmulatorPackageVersion = lineData.takeFirst();
                    if (sEmulatorPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
                        setEmulatorInstalled(true);
                        findEmulatorImages();
                        break;
                    }
                }
            }
            break;
        }
        case CreateEmulatorImage: {
            findEmulatorImages();
            break;
        }
        case FindImages: {
            QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));

            QSet<int> notFoundImages;
            foreach(UbuntuDevicesItem *item, m_knownDevices) {
                if(item->device()->machineType() == ProjectExplorer::IDevice::Emulator)
                    notFoundImages.insert(item->device()->id().uniqueIdentifier());
            }

            QMutableStringListIterator iter(lines);
            QRegularExpression regexName   (QStringLiteral("^(\\S+)"));
            QRegularExpression regexUbuntu (QStringLiteral("ubuntu=([0-9]+)"));
            QRegularExpression regexDevice (QStringLiteral("device=([0-9]+)"));
            QRegularExpression regexVersion(QStringLiteral("version=([0-9]+)"));
            QRegularExpression regexArch   (QStringLiteral("arch=(\\w+)"));
            while (iter.hasNext()) {
                QString line = iter.next();
                if(line.isEmpty()) {
                    iter.remove();
                    continue;
                }

                if(debug) qDebug()<<"Handling emulator: "<<line;

                QRegularExpressionMatch mName = regexName.match(line);
                QRegularExpressionMatch mUbu  = regexUbuntu.match(line);
                QRegularExpressionMatch mDev  = regexDevice.match(line);
                QRegularExpressionMatch mVer  = regexVersion.match(line);
                QRegularExpressionMatch mArch = regexArch.match(line);


                if(!mName.hasMatch())
                    continue;

                //emulators are identified by their image name
                QString deviceSerial = mName.captured(1);


                QString ubuntuVer = tr("unknown");
                QString deviceVer = tr("unknown");
                QString imageVer = tr("unknown");
                QString archType = QStringLiteral("i386");

                if(mUbu.hasMatch())
                    ubuntuVer = mUbu.captured(1);

                if(mDev.hasMatch())
                    deviceVer = mDev.captured(1);

                if(mVer.hasMatch())
                    imageVer = mVer.captured(1);

                if(mArch.hasMatch())
                    archType = mArch.captured(1);

                bool addToManager = false;
                Ubuntu::Internal::UbuntuDevice::Ptr dev;
                Core::Id devId = Core::Id::fromSetting(deviceSerial);
                int index = findDevice(devId.uniqueIdentifier());

                if(index >= 0) {
                    notFoundImages.remove(devId.uniqueIdentifier());
                    dev = m_knownDevices[index]->device();
                } else {
                    dev = Ubuntu::Internal::UbuntuDevice::create(
                                deviceSerial,
                                deviceSerial,
                                ProjectExplorer::IDevice::Emulator,
                                archType,
                                ProjectExplorer::IDevice::AutoDetected);

                    addToManager = true;
                }

                if(dev) {
                    dev->setEmulatorInfo(ubuntuVer,deviceVer,imageVer);

                    if(addToManager)
                        ProjectExplorer::DeviceManager::instance()->addDevice(dev);
                }
            }

            //remove all ubuntu emulators that are in the settings but don't exist in the system
            foreach(int curr,notFoundImages) {
                ProjectExplorer::DeviceManager::instance()->removeDevice(Core::Id::fromUniqueIdentifier(curr));
            }

            queryAdb();
            break;
        }
        case AdbList: {
            foreach(UbuntuDevicesItem* item, m_knownDevices)
                item->device()->helper()->refresh();

            if(exitCode != 0) {
                return;
            }

            QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
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

                        QRegularExpression archExp(QStringLiteral("arch:([\\w]+)"));
                        QRegularExpressionMatch archMatch = archExp.match(sDeviceInfo);

                        QString arch = QStringLiteral("armhf");
                        if(archMatch.hasMatch()) {
                            arch = archMatch.captured(1);
                        } else {
                            qDebug()<<"Could not get the architecture from: "<<sDeviceInfo<<" defaulting to armhf";
                        }


                        //sometimes the adb server is not started when adb devices is
                        //executed, we just skip those lines
                        if(sSerialNumber == QLatin1String("*"))
                            continue;

                        if(sSerialNumber.isEmpty() || sSerialNumber.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_ADB_NOACCESS))) {
                            continue;
                        }
                        if(sSerialNumber == QStringLiteral("ADB")) {
                            if(debug) qDebug()<<"Serialnumber ADB catched "<<m_reply;

                            continue;
                        }

                        registerNewDevice(sSerialNumber,arch);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    m_reply.clear();
}

/*!
 * \class UbuntuDevicesItem
 * Represents a Device inside the UbuntuDevicesModel,
 * the item tracks changes in the device and notifies the model
 * accordingly
 */
UbuntuDevicesItem::UbuntuDevicesItem(UbuntuDevice::Ptr device, QObject* parent) :
    QObject(parent),
    m_device(device)
{
    QList<ProjectExplorer::Kit*> allkits = ProjectExplorer::KitManager::kits();
    foreach (ProjectExplorer::Kit* k, allkits) {
        if( ProjectExplorer::DeviceKitInformation::deviceId(k) == device->id() )
            m_myKits.insert(k->id());
    }

    connect(ProjectExplorer::KitManager::instance(),SIGNAL(kitAdded(ProjectExplorer::Kit*)),
            this,SLOT(onKitAdded(ProjectExplorer::Kit*)));
    connect(ProjectExplorer::KitManager::instance(),SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this,SLOT(onKitRemoved(ProjectExplorer::Kit*)));
    connect(ProjectExplorer::KitManager::instance(),SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this,SLOT(onKitUpdated(ProjectExplorer::Kit*)));

    connect(device->helper(),SIGNAL(detectionStateChanged()),this,SLOT(deviceStateChanged()));
    connect(device->helper(),SIGNAL(featureDetected()),this,SIGNAL(featureDetected()));
    connect(device->helper(),SIGNAL(deviceInfoUpdated()),this,SIGNAL(deviceInfoUpdated()));
    connect(device->helper(),SIGNAL(message(QString)),this,SIGNAL(logUpdated()));
}

/*!
 * \brief UbuntuDevicesItem::id
 * Returns the internal device ID
 */
Core::Id UbuntuDevicesItem::id() const
{
    return m_device->id();
}

UbuntuDevice::Ptr UbuntuDevicesItem::device() const
{
    return m_device;
}

QVariantList UbuntuDevicesItem::kits() const
{
    QVariantList kits;
    foreach (const Core::Id &id, m_myKits) {
        ProjectExplorer::Kit* k = ProjectExplorer::KitManager::find(id);
        if(!k)
            continue;

        if(debug) qDebug()<<"Adding "<<k->displayName();

        QVariantMap m;
        m.insert(QStringLiteral("displayName"),k->displayName());
        m.insert(QStringLiteral("id"),k->id().toSetting());
        kits.append(QVariant::fromValue(m));
    }

    return kits;
}

/*!
 * \brief UbuntuDevicesItem::onKitAdded
 * Checks if the new Kit that just got added to the KitManager contains
 * the device, if it does the Kit is added to the internal Kit list
 */
void UbuntuDevicesItem::onKitAdded(ProjectExplorer::Kit *k)
{
    if( ProjectExplorer::DeviceKitInformation::deviceId(k) == m_device->id() ) {
        if(!m_myKits.contains(k->id())){
            m_myKits.insert(k->id());
            emit kitsChanged();
        }
    }
}

/*!
 * \brief UbuntuDevicesItem::onKitRemoved
 * Checks if the removed Kit \a k is in the internal Kit list
 * and removes it
 */
void UbuntuDevicesItem::onKitRemoved(ProjectExplorer::Kit *k)
{
    if(m_myKits.contains(k->id())) {
        m_myKits.remove(k->id());
        emit kitsChanged();
    }
}

/*!
 * \brief UbuntuDevicesItem::onKitUpdated
 * Checks if the updated Kit now contains the internal device
 * or if it does NOT contain the device anymore
 */
void UbuntuDevicesItem::onKitUpdated(ProjectExplorer::Kit *k)
{
    if( ProjectExplorer::DeviceKitInformation::deviceId(k) == m_device->id() ) {
        onKitAdded(k);
    } else {
        onKitRemoved(k);
    }
}

void UbuntuDevicesItem::deviceStateChanged()
{
    emit detectionStateChanged();
}

}
}

