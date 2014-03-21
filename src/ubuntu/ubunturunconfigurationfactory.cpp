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

#include "ubunturunconfigurationfactory.h"
#include "ubunturemoterunconfiguration.h"
#include "clicktoolchain.h"
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

QList<Core::Id> UbuntuRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const {
    if (!canHandle(parent))
        return QList<Core::Id>();

    QList<Core::Id> list;

    if(parent->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID) {
        list << UbuntuRemoteRunConfiguration::typeId();
    } else {
        list << Core::Id(Constants::UBUNTUPROJECT_RUNCONTROL_ID);
    }

    return list;
}

QString UbuntuRunConfigurationFactory::displayNameForId(const Core::Id id) const {
    if (id == Constants::UBUNTUPROJECT_RUNCONTROL_ID)
        return tr(Constants::UBUNTUPROJECT_DISPLAYNAME);
    else if(id == UbuntuRemoteRunConfiguration::typeId())
        return tr(Constants::UBUNTUPROJECT_DISPLAYNAME);
    return QString();
}

bool UbuntuRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                         const Core::Id id) const {
    if (!canHandle(parent))
        return false;

    if (id == Constants::UBUNTUPROJECT_RUNCONTROL_ID)
        return true;

    if (id == UbuntuRemoteRunConfiguration::typeId())
        return true;

    return false;
}

bool UbuntuRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const {
    return parent && canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::RunConfiguration *UbuntuRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, const Core::Id id) {
    if (!canCreate(parent, id))
        return NULL;

    if( id == UbuntuRemoteRunConfiguration::typeId() )
        return new UbuntuRemoteRunConfiguration(parent);

    return new UbuntuRunConfiguration(parent, id);
}

ProjectExplorer::RunConfiguration *UbuntuRunConfigurationFactory::doRestore(ProjectExplorer::Target *parent, const QVariantMap &map) {
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

bool UbuntuRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const {
    return canCreate(parent,product->id());
}

ProjectExplorer::RunConfiguration *UbuntuRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                   ProjectExplorer::RunConfiguration *source) {
    if (!canClone(parent, source))
        return NULL;

    if(source->id() == UbuntuRemoteRunConfiguration::typeId())
        return new UbuntuRemoteRunConfiguration(parent,static_cast<UbuntuRemoteRunConfiguration*>(source));

    return new UbuntuRunConfiguration(parent,static_cast<UbuntuRunConfiguration*>(source));
}

bool UbuntuRunConfigurationFactory::canHandle(ProjectExplorer::Target *parent) const {

    if(qobject_cast<CMakeProjectManager::CMakeProject*>(parent->project())) {
        if (!parent->project()->supportsKit(parent->kit()))
            return false;

        ProjectExplorer::ToolChain *tc
                = ProjectExplorer::ToolChainKitInformation::toolChain(parent->kit());
        if (!tc || tc->targetAbi().os() != ProjectExplorer::Abi::LinuxOS)
            return false;

        if(tc->type() != QString::fromLatin1(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
            return false;

        return true;
    }

    if (!qobject_cast<UbuntuProject *>(parent->project()))
        return false;
    return true;
}
