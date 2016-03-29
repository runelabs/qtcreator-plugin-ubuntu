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
#ifndef UBUNTU_INTERNAL_UBUNTUDEVICEFACTORY_H
#define UBUNTU_INTERNAL_UBUNTUDEVICEFACTORY_H

#include <projectexplorer/devicesupport/idevicefactory.h>

namespace Ubuntu {
namespace Internal {

class UbuntuDeviceFactory : public ProjectExplorer::IDeviceFactory
{
    Q_OBJECT
public:
    UbuntuDeviceFactory(QObject* parent = 0);

    // IDeviceFactory interface
    virtual QString displayNameForId(Core::Id type) const override;
    virtual QList<Core::Id> availableCreationIds() const override;
    virtual bool canCreate() const override;
    virtual ProjectExplorer::IDevice::Ptr create(Core::Id id) const override;
    virtual bool canRestore(const QVariantMap &map) const override;
    virtual ProjectExplorer::IDevice::Ptr restore(const QVariantMap &map) const override;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEVICEFACTORY_H
