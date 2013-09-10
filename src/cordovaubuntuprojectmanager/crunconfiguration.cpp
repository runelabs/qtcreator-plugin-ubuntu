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
 */

#include "crunconfiguration.h"
#include "constants.h"
#include "cproject.h"

namespace CordovaUbuntuProjectManager {

QList<Core::Id> CRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const {
    if (!canHandle(parent))
        return QList<Core::Id>();

    QList<Core::Id> list;
    list << Core::Id(RC_ID);

    return list;
}

QString CRunConfigurationFactory::displayNameForId(const Core::Id id) const {
    if (id == RC_ID)
        return tr("CordovaUbuntu");
    return QString();
}

bool CRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                         const Core::Id id) const {
    if (!canHandle(parent))
        return false;

    if (id == RC_ID)
        return true;

    return false;
}

ProjectExplorer::RunConfiguration *CRunConfigurationFactory::doCreate(ProjectExplorer::Target *parent, const Core::Id id) {
    if (!canCreate(parent, id))
        return nullptr;
    return new CRunConfiguration(parent, id);
}

bool CRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const {
    return parent && canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::RunConfiguration *CRunConfigurationFactory::doRestore(ProjectExplorer::Target *parent, const QVariantMap &map) {
    if (!canRestore(parent, map))
        return nullptr;

    return nullptr;
}

bool CRunConfigurationFactory::canClone(ProjectExplorer::Target *, ProjectExplorer::RunConfiguration *) const {
    return nullptr;
}

ProjectExplorer::RunConfiguration *CRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                   ProjectExplorer::RunConfiguration *source) {
    if (!canClone(parent, source))
        return nullptr;

    return nullptr;
}

bool CRunConfigurationFactory::canHandle(ProjectExplorer::Target *parent) const {
    if (!qobject_cast<CProject *>(parent->project()))
        return false;
    return true;
}

}
