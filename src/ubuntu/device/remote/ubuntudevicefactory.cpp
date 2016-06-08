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
#include "ubuntudevice.h"
#include <ubuntu/ubuntuconstants.h>

#include <utils/qtcassert.h>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

namespace Ubuntu {
namespace Internal {

//copied from IDevice
const char TypeKey[]        = "OsType";
const char MachineTypeKey[] = "Type";

UbuntuDeviceFactory::UbuntuDeviceFactory(QObject *parent) : ProjectExplorer::IDeviceFactory(parent)
{
}

} // namespace Internal
} // namespace Ubuntu


QString Ubuntu::Internal::UbuntuDeviceFactory::displayNameForId(Core::Id type) const
{
    QTC_ASSERT(type.toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID)), return QString());

    return tr("Ubuntu Device (%1)").arg(type.suffixAfter(Constants::UBUNTU_DEVICE_TYPE_ID));
}

QList<Core::Id> Ubuntu::Internal::UbuntuDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>()
            << Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix("armhf")
            << Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix("i386");
}

bool Ubuntu::Internal::UbuntuDeviceFactory::canCreate() const
{
    return false;
}

ProjectExplorer::IDevice::Ptr Ubuntu::Internal::UbuntuDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id);
    return UbuntuDevice::Ptr();
}

bool Ubuntu::Internal::UbuntuDeviceFactory::canRestore(const QVariantMap &map) const
{
    return ProjectExplorer::IDevice::typeFromMap(map).toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID));
}

ProjectExplorer::IDevice::Ptr Ubuntu::Internal::UbuntuDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return UbuntuDevice::Ptr());

    //fix up old device types
    if ( ProjectExplorer::IDevice::typeFromMap(map) == Constants::UBUNTU_DEVICE_TYPE_ID ) {
        //if those values are not available in the map we can not restore the devices anyway, its better to
        //return invalid device in that case
        QTC_ASSERT(map.contains(QLatin1String(TypeKey)), return UbuntuDevice::Ptr());
        QTC_ASSERT(map.contains(QLatin1String(MachineTypeKey)), return UbuntuDevice::Ptr());

        Core::Id newType;

        ProjectExplorer::IDevice::MachineType mType =
                static_cast<ProjectExplorer::IDevice::MachineType>(map.value(QLatin1String(MachineTypeKey),
                                                                             ProjectExplorer::IDevice::Hardware).toInt());
        if (mType == ProjectExplorer::IDevice::Emulator) {
            QString emuName = ProjectExplorer::IDevice::idFromMap(map).toSetting().toString();
            QString emuArchFileName = QStringLiteral("%1/ubuntu-emulator/%2/.device")
                    .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                    .arg(emuName);

            QString archType;
            if (QFile::exists(emuArchFileName)) {
                QFile emuArchFile(emuArchFileName);
                if (emuArchFile.open(QIODevice::ReadOnly)) {
                    QTextStream in(&emuArchFile);
                    archType = in.readAll().simplified();
                }
            }

            if (archType.isEmpty()) {
                //if there is no arch file, make a guess
                archType = QStringLiteral("i386");
            }

            newType = Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix(archType);
        } else {
            newType = Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix("armhf");
        }

        QVariantMap newMap = map;
        newMap[QLatin1String(TypeKey)] = newType.toString();

        const ProjectExplorer::IDevice::Ptr device = UbuntuDevice::create();
        device->fromMap(newMap);
        return device;
    }

    const ProjectExplorer::IDevice::Ptr device = UbuntuDevice::create();
    device->fromMap(map);
    return device;
}
