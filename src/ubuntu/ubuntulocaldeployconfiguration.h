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

#ifndef UBUNTU_INTERNAL_UBUNTULOCALDEPLOYCONFIGURATIONFACTORY_H
#define UBUNTU_INTERNAL_UBUNTULOCALDEPLOYCONFIGURATIONFACTORY_H

#include <projectexplorer/deployconfiguration.h>

namespace Ubuntu {
namespace Internal {

class UbuntuLocalDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit UbuntuLocalDeployConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const override;
    QString displayNameForId(const Core::Id id) const override;
    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const override;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id) override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *product) override;
};

class UbuntuLocalDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    friend class UbuntuLocalDeployConfigurationFactory; // for the ctors
protected:
    UbuntuLocalDeployConfiguration(ProjectExplorer::Target *target, const Core::Id id);
    UbuntuLocalDeployConfiguration(ProjectExplorer::Target *target, UbuntuLocalDeployConfiguration *source);

protected slots:
    void selectAsDefaultHack();
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTULOCALDEPLOYCONFIGURATIONFACTORY_H
