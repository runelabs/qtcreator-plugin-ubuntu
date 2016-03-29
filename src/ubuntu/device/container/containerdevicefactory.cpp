#include "containerdevicefactory.h"
#include "containerdevice.h"
#include "../../ubuntuconstants.h"
#include "../../ubuntuclicktool.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QProcess>

namespace Ubuntu {
namespace Internal {

ContainerDeviceFactory::ContainerDeviceFactory(QObject *parent) : IDeviceFactory(parent)
{ }

QString ContainerDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type.toString().startsWith(QLatin1String(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID))) {
        return QStringLiteral("Ubuntu Local Device (")
                + type.suffixAfter(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID)
                + QStringLiteral(")");
    }
    return QString();
}

QList<Core::Id> ContainerDeviceFactory::availableCreationIds() const
{
    QList<Core::Id> deviceIds;
    QList<UbuntuClickTool::Target> targets = UbuntuClickTool::listPossibleDeviceContainers();

    foreach(const UbuntuClickTool::Target &t, targets) {
        deviceIds.append(ContainerDevice::createIdForContainer(t.containerName));
    }

    return deviceIds;
}

ProjectExplorer::IDevice::Ptr ContainerDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id);
    return ProjectExplorer::IDevice::Ptr();
}

bool ContainerDeviceFactory::canRestore(const QVariantMap &map) const
{
    //we can only restore devices that have a existing container
    return availableCreationIds().contains(ProjectExplorer::IDevice::idFromMap(map));
}

ProjectExplorer::IDevice::Ptr ContainerDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return ProjectExplorer::IDevice::Ptr());

    Core::Id typeId = ContainerDevice::typeFromMap(map);
    Core::Id devId  = ContainerDevice::idFromMap(map);

    if (!typeId.isValid() || !devId.isValid())
        return ProjectExplorer::IDevice::Ptr();

    const ProjectExplorer::IDevice::Ptr device
            = ProjectExplorer::IDevice::Ptr(new ContainerDevice(typeId, devId));
    device->fromMap(map);
    return device;
}

bool ContainerDeviceFactory::canCreate() const
{
    return false;
}

} // namespace Internal
} // namespace Ubuntu

