#include "ubuntudevicefactory.h"
#include "ubuntuconstants.h"
#include "ubuntudevice.h"

#include <utils/qtcassert.h>

namespace Ubuntu {
namespace Internal {

UbuntuDeviceFactory::UbuntuDeviceFactory(QObject *parent) : ProjectExplorer::IDeviceFactory(parent)
{
}

} // namespace Internal
} // namespace Ubuntu


QString Ubuntu::Internal::UbuntuDeviceFactory::displayNameForId(Core::Id type) const
{
    QTC_ASSERT(type == Constants::UBUNTU_DEVICE_TYPE_ID, return QString());
    return tr("Ubuntu Device");
}

QList<Core::Id> Ubuntu::Internal::UbuntuDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID);
}

bool Ubuntu::Internal::UbuntuDeviceFactory::canCreate() const
{
    return false;
}

ProjectExplorer::IDevice::Ptr Ubuntu::Internal::UbuntuDeviceFactory::create(Core::Id id) const
{
    return UbuntuDevice::Ptr();
}

bool Ubuntu::Internal::UbuntuDeviceFactory::canRestore(const QVariantMap &map) const
{
    return false;
}

ProjectExplorer::IDevice::Ptr Ubuntu::Internal::UbuntuDeviceFactory::restore(const QVariantMap &map) const
{
    return UbuntuDevice::Ptr ();
}
