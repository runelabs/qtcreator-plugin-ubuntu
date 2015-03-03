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
#include "ubuntulocaldeployconfiguration.h"
#include "ubuntuprojecthelper.h"
#include "ubuntuconstants.h"
#include "ubuntupackagestep.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/buildsteplist.h>

#include <QTimer>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuLocalDeployConfigurationFactory
 * Creates the required deploy configuration for locally
 * run click apps. Click apps need a extra step to group
 * all components into one directory and make them easily executable
 *
 * \note NOT IN USE CURRENTLY
 */
UbuntuLocalDeployConfigurationFactory::UbuntuLocalDeployConfigurationFactory(QObject *parent)
    :DeployConfigurationFactory(parent)
{

}

QList<Core::Id> UbuntuLocalDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;

    if(ProjectExplorer::DeviceKitInformation::deviceId(parent->kit()) != ProjectExplorer::Constants::DESKTOP_DEVICE_ID)
        return ids;

    ids << Core::Id(Constants::UBUNTU_LOCAL_DEPLOYCONFIGURATION_ID);

    return ids;
}

QString UbuntuLocalDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if( Core::Id(Constants::UBUNTU_LOCAL_DEPLOYCONFIGURATION_ID) == id )
        return tr("Ubuntu SDK Local Deployment");

    return QString();
}

bool UbuntuLocalDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::DeployConfiguration *UbuntuLocalDeployConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id)
{
    if(!canCreate(parent,id))
        return 0;

    UbuntuLocalDeployConfiguration* conf = new UbuntuLocalDeployConfiguration(parent,id);
    ProjectExplorer::BuildStepList* steps = conf->stepList();

    UbuntuPackageStep* depl = new UbuntuPackageStep(steps);
    steps->insertStep(0,depl);

    return conf;
}

bool UbuntuLocalDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *UbuntuLocalDeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = ProjectExplorer::idFromMap(map);

    UbuntuLocalDeployConfiguration* const dc = new UbuntuLocalDeployConfiguration(parent, id);
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

ProjectExplorer::DeployConfiguration *UbuntuLocalDeployConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new UbuntuLocalDeployConfiguration(parent,qobject_cast<UbuntuLocalDeployConfiguration *>(product));
}


///
// UbuntuLocalDeployConfiguration
///
UbuntuLocalDeployConfiguration::UbuntuLocalDeployConfiguration(ProjectExplorer::Target *target, const Core::Id id)
    : DeployConfiguration(target, id)
{
    setDefaultDisplayName(tr("UbuntuSDK Deploy locally"));

    //this should run after the configurations are set up
    //and selects us as the default one, its ugly but works
    QTimer::singleShot(0,this,SLOT(selectAsDefaultHack()));
}

UbuntuLocalDeployConfiguration::UbuntuLocalDeployConfiguration(ProjectExplorer::Target *target, UbuntuLocalDeployConfiguration *source)
    : DeployConfiguration(target, source)
{
    cloneSteps(source);
}

void UbuntuLocalDeployConfiguration::selectAsDefaultHack()
{
    target()->setActiveDeployConfiguration(this);
}

} // namespace Internal
} // namespace Ubuntu
