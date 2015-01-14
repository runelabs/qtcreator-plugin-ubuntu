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
#include "ubuntuprojecthelper.h"
#include "ubuntudevice.h"
#include "clicktoolchain.h"
#include "ubuntuclickmanifest.h"
#include "ubuntucmakecache.h"
#include "ubuntushared.h"

#include <projectexplorer/taskhub.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

enum {
    debug = 0
};

QList<Core::Id> UbuntuLocalRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> types;

    Core::Id targetDevice = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if(targetDevice != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE && !targetDevice.toString().startsWith(QLatin1String(Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID))) {
        if(debug) qDebug()<<"Rejecting device type: "<<targetDevice.toString();
        return types;
    }

    bool isRemote = targetDevice.toString().startsWith(QLatin1String(Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID));
    bool isCMake  = parent->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID;
    bool isHTML   = parent->project()->id() == Ubuntu::Constants::UBUNTUPROJECT_ID;
    bool isQML    = parent->project()->id() == "QmlProjectManager.QmlProject";
    bool isQMake  = parent->project()->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID;

    if (!isCMake && !isHTML &&!isQML &&!isQMake)
        return types;

    if (isRemote) {
        //IF we have a remote device we just support a ubuntu toolchain
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(parent->kit());
        if(tc && tc->type() != QLatin1String(Ubuntu::Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
            return types;
    }

    QString defaultPath = QDir::cleanPath(parent->project()->projectDirectory()
                                          +QDir::separator()
                                          +QStringLiteral("manifest.json"));

    QString manifestPath = UbuntuProjectHelper::getManifestPath(parent,defaultPath);

    qDebug()<<"Using the manifest path: "<<manifestPath;

    //if we have no manifest, we can not query the app id's
    if(!QFile::exists(manifestPath))
        return types;

    QString err;
    UbuntuClickManifest manifest;
    if(!manifest.load(manifestPath,nullptr,&err)) {
        ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Warning,tr("Can not read manifest.json file: %1").arg(err),ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM
                                          ,Utils::FileName::fromString(manifestPath));
        return types;
    }

    QList<UbuntuClickManifest::Hook> hooks = manifest.hooks();
    if (!isRemote) {
        foreach (const UbuntuClickManifest::Hook &hook, hooks) {
            if(hook.type() == UbuntuClickManifest::Hook::Application)
                types << Core::Id(Constants::UBUNTUPROJECT_RUNCONTROL_APP_ID).withSuffix(hook.appId);
            else if (hook.type() == UbuntuClickManifest::Hook::Scope)
                types << Core::Id(Constants::UBUNTUPROJECT_RUNCONTROL_SCOPE_ID).withSuffix(hook.appId);
        }
    }
    else if (isRemote) {
        if (isCMake || isHTML || isQML || isQMake) {
            foreach (const UbuntuClickManifest::Hook &hook, hooks) {
                if(hook.type() == UbuntuClickManifest::Hook::Application)
                    types << Core::Id(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_APP_ID).withSuffix(hook.appId);
                else if (hook.type() == UbuntuClickManifest::Hook::Scope)
                    types << Core::Id(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_SCOPE_ID).withSuffix(hook.appId);
            }
        }
    }

    return types;
}

QString UbuntuLocalRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_APP_ID )))
        return id.suffixAfter(Constants::UBUNTUPROJECT_RUNCONTROL_APP_ID);
    else if (id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_SCOPE_ID )))
        return id.suffixAfter(Constants::UBUNTUPROJECT_RUNCONTROL_SCOPE_ID);
    else if (id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_APP_ID )))
        return id.suffixAfter(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_APP_ID);
    else if (id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_SCOPE_ID )))
        return id.suffixAfter(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_SCOPE_ID);
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

    if ( id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_BASE_ID)))
        return new UbuntuRemoteRunConfiguration(parent, id);
    else if (id.toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_BASE_ID)))
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

    if(source->id().toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_BASE_ID)))
        return new UbuntuRemoteRunConfiguration(parent,static_cast<UbuntuRemoteRunConfiguration*>(source));

    return new UbuntuLocalRunConfiguration(parent,static_cast<UbuntuLocalRunConfiguration*>(source));
}
