#include "ubuntudevicesmodel.h"
#include "ubuntuconstants.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>

#include <QVariant>

namespace Ubuntu {
namespace Internal {

UbuntuDevicesModel::UbuntuDevicesModel(QObject *parent) :
    QAbstractListModel(parent)
{
    ProjectExplorer::KitManager* devMgr = static_cast<ProjectExplorer::KitManager*>(ProjectExplorer::KitManager::instance());
    connect(devMgr,SIGNAL(kitsLoaded()),this,SLOT(readDevicesFromSettings()));
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
    return QAbstractListModel::setData(index,value,role);
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
            break;
        case ConnectionStateRole:
            return m_knownDevices[index.row()]->device()->deviceStateToString();
            break;
        case KitListRole:
            return QVariant::fromValue(m_knownDevices[index.row()]->kits());
            break;
        case DeveloperModeRole:
            return m_knownDevices[index.row()]->device()->developerModeEnabled();
            break;
        case NetworkConnectionRole:
            return m_knownDevices[index.row()]->device()->hasNetworkConnection();
            break;
        case WriteableImageRole:
            return m_knownDevices[index.row()]->device()->hasWriteableImage();
            break;
        case DeveloperToolsRole:
            return m_knownDevices[index.row()]->device()->hasDeveloperTools();
            break;
        default:
            break;
    }

    return QVariant();
}

QHash<int, QByteArray> UbuntuDevicesModel::roleNames() const
{
    QHash<int,QByteArray> roles = QAbstractListModel::roleNames();
    roles.insert(ConnectionStateRole,"connectionState");
    roles.insert(KitListRole,"kits");
    roles.insert(DeveloperModeRole,"developerModeEnabled");
    roles.insert(NetworkConnectionRole,"hasNetworkConnection");
    roles.insert(WriteableImageRole,"hasWriteableImage");
    roles.insert(DeveloperToolsRole,"hasDeveloperTools");
    return roles;
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

UbuntuDevicesItem *UbuntuDevicesModel::createItem(UbuntuDevice::Ptr dev)
{
    UbuntuDevicesItem *devItem = new UbuntuDevicesItem(dev,this);
    connect(devItem,SIGNAL(kitsChanged()),this,SLOT(deviceDataChanged()));
    connect(devItem,SIGNAL(detectionStateChanged()),this,SLOT(deviceDataChanged()));

    return devItem;
}

/*!
 * \brief UbuntuDevicesWidget::readDevicesFromSettings
 * read all known devices from the DeviceManager, this is triggered
 * automatically on startup
 */
void UbuntuDevicesModel::readDevicesFromSettings()
{
    if(rowCount()) {
        beginRemoveRows(QModelIndex(),0,rowCount()-1);
        qDeleteAll(m_knownDevices.begin(),m_knownDevices.end());
        m_knownDevices.clear();
        endRemoveRows();
    }

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

void UbuntuDevicesModel::deviceDataChanged()
{
    UbuntuDevicesItem* hlpr = qobject_cast<UbuntuDevicesItem*>(sender());
    if(!hlpr) return;
    int index = findDevice(hlpr->id().uniqueIdentifier());
    if(index >= 0)
        emit dataChanged(createIndex(index,0),createIndex(index,0));
}

/*!
 * \brief UbuntuDevicesWidget::deviceAdded
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
 * \brief UbuntuDevicesWidget::deviceUpdated
 * called when a device state is changed between connected
 * and disconnected, adds or removes the config widget
 * accordingly
 */
void UbuntuDevicesModel::deviceUpdated(const Core::Id &id)
{
    if (hasDevice(id.uniqueIdentifier()))
        return;
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
        m.insert(QLatin1String("displayName"),k->displayName());
        m.insert(QLatin1String("id"),k->id().toSetting());
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

