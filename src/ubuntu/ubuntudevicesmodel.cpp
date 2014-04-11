#include "ubuntudevicesmodel.h"
#include "ubuntuconstants.h"

#include <projectexplorer/devicesupport/devicemanager.h>

namespace Ubuntu {
namespace Internal {

UbuntuDevicesModel::UbuntuDevicesModel(QObject *parent) :
    QAbstractListModel(parent)
{
    ProjectExplorer::DeviceManager* devMgr = ProjectExplorer::DeviceManager::instance();
    connect(devMgr,SIGNAL(devicesLoaded()),this,SLOT(readDevicesFromSettings()));
}

int UbuntuDevicesModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;
    return m_knownDevices.size();
}

QVariant UbuntuDevicesModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()
            || index.parent().isValid()
            || index.row() < 0
            || index.row() > rowCount())
        return QVariant();

    if( Qt::DisplayRole == role || Qt::EditRole == role ) {
        QString id = m_knownDevices[index.row()]->id().toString();
        QString conn = m_knownDevices[index.row()]->deviceStateToString();

        return QString(id + QLatin1String(" ") + conn);
    }

    return QVariant();
}

QHash<int, QByteArray> UbuntuDevicesModel::roleNames() const
{
    QHash<int,QByteArray> roles = QAbstractListModel::roleNames();
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

/*!
 * \brief UbuntuDevicesWidget::readDevicesFromSettings
 * read all known devices from the DeviceManager, this is triggered
 * automatically on startup
 */
void UbuntuDevicesModel::readDevicesFromSettings()
{
    if(rowCount()) {
        beginRemoveRows(QModelIndex(),0,rowCount()-1);
        m_knownDevices.clear();
        endRemoveRows();
    }

    QList<Ubuntu::Internal::UbuntuDevice::Ptr> devs;
    ProjectExplorer::DeviceManager* devMgr = ProjectExplorer::DeviceManager::instance();
    for(int i = 0; i < devMgr->deviceCount(); i++) {
        ProjectExplorer::IDevice::ConstPtr dev = devMgr->deviceAt(i);
        if(dev && dev->type() == Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID)) {

            //ugly hack to get a mutable version of the device
            //no idea why its necessary to lock it up
            Ubuntu::Internal::UbuntuDevice* cPtr = qSharedPointerCast<const Ubuntu::Internal::UbuntuDevice>(dev)->helper()->device();
            if(cPtr) {
                devs.append(cPtr->sharedFromThis());
                connect(cPtr->helper(),SIGNAL(connected()),this,SLOT(deviceDataChanged()));
                connect(cPtr->helper(),SIGNAL(disconnected()),this,SLOT(deviceDataChanged()));
                connect(cPtr->helper(),SIGNAL(detectionStateChanged()),this,SLOT(deviceDataChanged()));
            }
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
    Ubuntu::Internal::UbuntuDeviceHelper* hlpr = qobject_cast<Ubuntu::Internal::UbuntuDeviceHelper*>(sender());
    if(!hlpr) return;
    int index = findDevice(hlpr->device()->id().uniqueIdentifier());
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
    m_knownDevices.append(dev->sharedFromThis());
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
    m_knownDevices.removeAt(index);
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

}
}

