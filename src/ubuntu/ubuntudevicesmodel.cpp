#include "ubuntudevicesmodel.h"
#include "ubuntuconstants.h"
#include "ubuntukitmanager.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QVariant>

namespace Ubuntu {
namespace Internal {

UbuntuDevicesModel::UbuntuDevicesModel(QObject *parent) :
    QAbstractListModel(parent)
{
    m_deviceNotifier = new UbuntuDeviceNotifier(this);
    connect(m_deviceNotifier,SIGNAL(deviceConnected(QString)),this,SLOT(deviceConnected(QString)));

    m_adbProcess = new QProcess(this);
    connect(m_adbProcess,SIGNAL(finished(int)),this,SLOT(refreshFinished(int)));
    connect(m_adbProcess,SIGNAL(readyRead()),this,SLOT(adbReadyRead()));


    ProjectExplorer::KitManager* devMgr = static_cast<ProjectExplorer::KitManager*>(ProjectExplorer::KitManager::instance());
    connect(devMgr,SIGNAL(kitsLoaded()),this,SLOT(readDevicesFromSettings()));
    connect(ProjectExplorer::DeviceManager::instance(),SIGNAL(deviceListReplaced()),this,SLOT(refresh()));
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
    qDebug()<<"Setting index "<<index<<" with data "<<value<<" to role "<<role;
    if(!index.isValid()
            || index.parent().isValid()
            || index.row() < 0
            || index.row() > rowCount())
        return false;

    UbuntuDevice::Ptr dev = m_knownDevices[index.row()]->device();

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return false;
        case KitListRole:
            return false;
        case DeveloperModeRole: {
            if(!value.type() == QVariant::Bool)
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
            if(!value.type() == QVariant::Bool)
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
            if(!value.type() == QVariant::Bool)
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
            if(!value.type() == QVariant::Bool)
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
        case EmulatorRole:
            return false;
        case LogRole:
            return m_knownDevices[index.row()]->device()->helper()->log();
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
    roles.insert(EmulatorRole,"isEmulator");
    roles.insert(LogRole,"deviceLog");
    return roles;
}

Qt::ItemFlags UbuntuDevicesModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

void UbuntuDevicesModel::triggerCloneTimeConfig(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->cloneTimeConfig();
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

void UbuntuDevicesModel::triggerReboot(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->reboot();
}

void UbuntuDevicesModel::triggerRebootBootloader(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->rebootToBootloader();
}

void UbuntuDevicesModel::triggerRebootRecovery(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->rebootToRecovery();
}

void UbuntuDevicesModel::triggerShutdown(const int devId)
{
    int row = findDevice(devId);
    if(row < 0)
        return;
    m_knownDevices[row]->device()->shutdown();
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
    if(ProjectExplorer::DeviceKitInformation::deviceId(k) == Core::Id(devId)) {
        //completely delete the kit
        ProjectExplorer::KitManager::deregisterKit(k);
    }
}

void UbuntuDevicesModel::refresh()
{
    if( m_adbProcess->state() != QProcess::NotRunning ) {
        //make sure we use a clean QProcess
        m_adbProcess->disconnect(this);
        m_adbProcess->kill();
        m_adbProcess->deleteLater();

        m_adbProcess = new QProcess(this);
        connect(m_adbProcess,SIGNAL(finished(int)),this,SLOT(refreshFinished(int)));
        connect(m_adbProcess,SIGNAL(readyRead()),this,SLOT(adbReadyRead()));
    }
    bool restartAdb = true;
    m_adbProcess->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_adbProcess->setArguments( QStringList() << (restartAdb ? QString::number(1) : QString::number(0)) );
    m_adbProcess->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICES_SCRIPT)
                          .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH));

    clear();
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
        if(dev && dev->type() == Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID)) {

            //ugly hack to get a mutable version of the device
            //no idea why its necessary to lock it up
            Ubuntu::Internal::UbuntuDevice* cPtr = qSharedPointerCast<const Ubuntu::Internal::UbuntuDevice>(dev)->helper()->device();
            if(cPtr)
                devs.append(createItem(cPtr->sharedFromThis()));

        }
    }

    beginInsertRows(QModelIndex(),0,devs.count()-1);
    m_knownDevices = devs;
    endInsertRows();

    connect(devMgr,SIGNAL(deviceAdded(Core::Id)),this,SLOT(deviceAdded(Core::Id)));
    connect(devMgr,SIGNAL(deviceRemoved(Core::Id)),this,SLOT(deviceRemoved(Core::Id)));
    connect(devMgr,SIGNAL(deviceUpdated(Core::Id)),this,SLOT(deviceUpdated(Core::Id)));
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

    if(ptr->type() != Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID))
        return;

    qDebug()<<"Device Manager reports device added: "<<id.toString();
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

    qDebug()<<"Device Manager reports device removed: "<<id.toString();
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

void UbuntuDevicesModel::refreshFinished(int exitCode)
{
    readDevicesFromSettings();
    foreach(UbuntuDevicesItem* item, m_knownDevices)
        item->device()->helper()->refresh();

    if(exitCode != 0) {
        return;
    }

    //read all data from the process
    adbReadyRead();

    QStringList lines = m_adbReply.trimmed().split(QLatin1String(Constants::LINEFEED));
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
}

void UbuntuDevicesModel::adbReadyRead()
{
    QString stderr = QString::fromLocal8Bit(m_adbProcess->readAllStandardError());
    QString stdout = QString::fromLocal8Bit(m_adbProcess->readAllStandardOutput());
    m_adbReply.append(stderr);
    m_adbReply.append(stdout);
}

/*!
 * \brief UbuntuDevicesModel::registerNewDevice
 * Registers a new device in the device manager if its not
 * already in the known devices map.
 * \note will not add it into model list, this
 *       will happen automatically when the device is
 *       registered in the device manager
 */
void UbuntuDevicesModel::registerNewDevice(const QString &serial, const QString &deviceInfo)
{
    if(findDevice(Core::Id::fromSetting(serial).uniqueIdentifier()) >= 0)
        return;

    bool isEmu = serial.startsWith(QLatin1String("emulator"));

    Ubuntu::Internal::UbuntuDevice::Ptr dev = Ubuntu::Internal::UbuntuDevice::create(
                tr("Ubuntu Device")
                , serial
                , isEmu ? ProjectExplorer::IDevice::Emulator : ProjectExplorer::IDevice::Hardware
                          , ProjectExplorer::IDevice::AutoDetected);

    dev->setDeviceInfoString(deviceInfo);
    ProjectExplorer::DeviceManager::instance()->addDevice(dev);
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

        qDebug()<<"Adding "<<k->displayName();

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

