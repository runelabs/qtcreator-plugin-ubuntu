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
    return ProjectExplorer::IDevice::typeFromMap(map) == Constants::UBUNTU_DEVICE_TYPE_ID;
}

ProjectExplorer::IDevice::Ptr Ubuntu::Internal::UbuntuDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return UbuntuDevice::Ptr());
    const ProjectExplorer::IDevice::Ptr device = UbuntuDevice::create();
    device->fromMap(map);
    return device;
}
