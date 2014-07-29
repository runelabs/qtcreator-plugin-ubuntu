/*
 * Copyright 2013 Canonical Ltd.
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
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#include "ubuntulocalrunconfigurationfactory.h"
#include "ubunturemoterunconfiguration.h"
#include "ubuntuprojectguesser.h"
#include "ubuntudevice.h"
#include "clicktoolchain.h"
#include "ubuntuclickmanifest.h"
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

enum {
    debug = 1
};

QList<Core::Id> UbuntuLocalRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> types;

    Core::Id targetDevice = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if(targetDevice != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE && targetDevice != Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID) {
        if(debug) qDebug()<<"Rejecting device type: "<<targetDevice.toString();
        return types;
    }

    bool isRemote = targetDevice == Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID;
    bool isCMake  = parent->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID;
    bool isHTML   = parent->project()->id() == Ubuntu::Constants::UBUNTUPROJECT_ID;
    bool isApp    = UbuntuProjectGuesser::isClickAppProject(parent->project());
    bool isQML    = parent->project()->id() == "QmlProjectManager.QmlProject";

    if (!isCMake && !isHTML &&!isQML)
        return types;

    if (isRemote) {
        //IF we have a remote device we just support a ubuntu toolchain
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(parent->kit());
        if(tc && tc->type() != QLatin1String(Ubuntu::Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
            return types;
    }

    QString manifestPath = QStringLiteral("%1/manifest.json").arg(parent->project()->projectDirectory());
    UbuntuClickManifest manifest;

    //if we have no manifest, we can not query the app id's
    if(!manifest.load(manifestPath,parent->displayName()))
        return types;

    QList<UbuntuClickManifest::Hook> hooks = manifest.hooks();
    if (!isRemote) {
        foreach (const UbuntuClickManifest::Hook &hook, hooks) {
            types << Core::Id(Constants::UBUNTUPROJECT_RUNCONTROL_ID).withSuffix(hook.appId);
        }
    }
    else if (isRemote) {
        //when CMake we only support App projects, scopes have no way to be controlled on the device atm
        if ((isCMake && isApp) || isHTML || isQML) {
            foreach (const UbuntuClickManifest::Hook &hook, hooks) {
                types << UbuntuRemoteRunConfiguration::typeId().withSuffix(hook.appId);
            }
        }
    }

    return types;
}

QString UbuntuLocalRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_ID )))
        return id.suffixAfter(Constants::UBUNTUPROJECT_RUNCONTROL_ID);
    else if(id.toString().startsWith(UbuntuRemoteRunConfiguration::typeId().toString()))
        return id.suffixAfter(UbuntuRemoteRunConfiguration::typeId());
    return QString();
}

bool UbuntuLocalRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                                   const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

bool UbuntuLocalRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const {
    return parent && canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::RunConfiguration *UbuntuLocalRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, const Core::Id id) {
    if (!canCreate(parent, id))
        return NULL;

    if ( id.toString().startsWith(UbuntuRemoteRunConfiguration::typeId().toString()) )
        return new UbuntuRemoteRunConfiguration(parent, id);
    else if (id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_ID)))
        return new UbuntuLocalRunConfiguration(parent, id);

    return 0;
}

ProjectExplorer::RunConfiguration *UbuntuLocalRunConfigurationFactory::doRestore(ProjectExplorer::Target *parent, const QVariantMap &map) {
    if (!canRestore(parent, map))
        return NULL;

    ProjectExplorer::RunConfiguration *conf = create(parent,ProjectExplorer::idFromMap(map));
    if(!conf)
        return NULL;
    if(!conf->fromMap(map)) {
        delete conf;
        return NULL;
    }
    return conf;
}

bool UbuntuLocalRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const {
    return canCreate(parent,product->id());
}

ProjectExplorer::RunConfiguration *UbuntuLocalRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                             ProjectExplorer::RunConfiguration *source) {
    if (!canClone(parent, source))
        return NULL;

    if(source->id().toString().startsWith(UbuntuRemoteRunConfiguration::typeId().toString()))
        return new UbuntuRemoteRunConfiguration(parent,static_cast<UbuntuRemoteRunConfiguration*>(source));

    return new UbuntuLocalRunConfiguration(parent,static_cast<UbuntuLocalRunConfiguration*>(source));
}
