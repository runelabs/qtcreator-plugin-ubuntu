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

using namespace Ubuntu;
using namespace Ubuntu::Internal;

QList<Core::Id> UbuntuRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const {
    if (!canHandle(parent))
        return QList<Core::Id>();

    QList<Core::Id> list;
    list << Core::Id(Constants::UBUNTUPROJECT_RUNCONTROL_ID);

    return list;
}

QString UbuntuRunConfigurationFactory::displayNameForId(const Core::Id id) const {
    if (id == Constants::UBUNTUPROJECT_RUNCONTROL_ID)
        return tr(Constants::UBUNTUPROJECT_DISPLAYNAME);
    return QString();
}

bool UbuntuRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                         const Core::Id id) const {
    if (!canHandle(parent))
        return false;

    if (id == Constants::UBUNTUPROJECT_RUNCONTROL_ID)
        return true;

    return false;
}

ProjectExplorer::RunConfiguration *UbuntuRunConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id) {
    if (!canCreate(parent, id))
        return NULL;
    return new UbuntuRunConfiguration(parent, id);
}

bool UbuntuRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const {
    return parent && canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::RunConfiguration *UbuntuRunConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map) {
    if (!canRestore(parent, map))
        return NULL;

    return NULL;
}

bool UbuntuRunConfigurationFactory::canClone(ProjectExplorer::Target *, ProjectExplorer::RunConfiguration *) const {
    return NULL;
}

ProjectExplorer::RunConfiguration *UbuntuRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                   ProjectExplorer::RunConfiguration *source) {
    if (!canClone(parent, source))
        return NULL;
    return NULL;
}

bool UbuntuRunConfigurationFactory::canHandle(ProjectExplorer::Target *parent) const {
    if (!qobject_cast<UbuntuProject *>(parent->project()))
        return false;
    return true;
}
