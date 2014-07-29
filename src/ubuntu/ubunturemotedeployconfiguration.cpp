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
#include "ubunturemotedeployconfiguration.h"
#include "ubuntucmakebuildconfiguration.h"
#include "ubuntucmakemakestep.h"
#include "ubuntudirectuploadstep.h"
#include "ubuntuprojectguesser.h"
#include "ubuntuconstants.h"
#include "ubuntupackagestep.h"

#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildstep.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>

#include <QDir>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuRemoteDeployConfiguration::UbuntuRemoteDeployConfiguration(ProjectExplorer::Target *target)
    : RemoteLinux::RemoteLinuxDeployConfiguration(target,Constants::UBUNTU_DEPLOYCONFIGURATION_ID,QString())
{
    setDefaultDisplayName(tr("Deploy to Ubuntu Device"));
}

UbuntuRemoteDeployConfiguration::UbuntuRemoteDeployConfiguration(ProjectExplorer::Target *target, UbuntuRemoteDeployConfiguration *source) :
    RemoteLinux::RemoteLinuxDeployConfiguration(target,source)
{

}

ProjectExplorer::NamedWidget *UbuntuRemoteDeployConfiguration::createConfigWidget()
{
    return new ProjectExplorer::NamedWidget();
}


UbuntuRemoteDeployConfigurationFactory::UbuntuRemoteDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("UbuntuDeployConfiguration"));
}

QList<Core::Id> UbuntuRemoteDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;

    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(parent->kit());
    if (!tc || tc->targetAbi().os() != ProjectExplorer::Abi::LinuxOS)
        return ids;

    Core::Id projectTypeId = parent->project()->id();

    //for now only support cmake and ubuntu projects
    if(projectTypeId != CMakeProjectManager::Constants::CMAKEPROJECT_ID
            && projectTypeId != Ubuntu::Constants::UBUNTUPROJECT_ID
            && projectTypeId != "QmlProjectManager.QmlProject")
        return ids;

    const Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (devType == Constants::UBUNTU_DEVICE_TYPE_ID && !UbuntuProjectGuesser::isScopesProject(parent->project()))
        ids << Core::Id(Constants::UBUNTU_DEPLOYCONFIGURATION_ID);

    return ids;
}

QString UbuntuRemoteDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::UBUNTU_DEPLOYCONFIGURATION_ID))
        return tr("Deploy to Ubuntu Device");
    return QString();
}

bool UbuntuRemoteDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::DeployConfiguration *UbuntuRemoteDeployConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                               const Core::Id id)
{
    QTC_ASSERT(canCreate(parent, id),return 0);

    ProjectExplorer::DeployConfiguration * const dc
            = new UbuntuRemoteDeployConfiguration(parent);

    int step = 0;

    UbuntuPackageStep *pckStep = new UbuntuPackageStep(dc->stepList());
    dc->stepList()->insertStep(0,pckStep);

    RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep *checkSpace = new RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList());
    checkSpace->setPathToCheck(QStringLiteral("/tmp"));
    dc->stepList()->insertStep(step+1, checkSpace);

    UbuntuDirectUploadStep* upload = new UbuntuDirectUploadStep(dc->stepList());
    dc->stepList()->insertStep(step+2,upload);
    return dc;
}

bool UbuntuRemoteDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *UbuntuRemoteDeployConfigurationFactory::restore(ProjectExplorer::Target *parent,
                                                                                const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    RemoteLinux::RemoteLinuxDeployConfiguration * const dc
            = new UbuntuRemoteDeployConfiguration(parent);

    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

ProjectExplorer::DeployConfiguration *UbuntuRemoteDeployConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                              ProjectExplorer::DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new RemoteLinux::RemoteLinuxDeployConfiguration(parent
                                                           ,qobject_cast<RemoteLinux::RemoteLinuxDeployConfiguration *>(product));
}

} // namespace Internal
} // namespace Ubuntu
