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

#ifndef CRUNCONFIGURATION_H
#define CRUNCONFIGURATION_H

#include "common.h"

namespace CordovaUbuntuProjectManager {

class CRunConfiguration : public ProjectExplorer::RunConfiguration {
    Q_OBJECT
public:
    CRunConfiguration(ProjectExplorer::Target *parent, Core::Id id) : ProjectExplorer::RunConfiguration(parent, id) {}

    QWidget *createConfigurationWidget() {
        return NULL;
    }

    bool isEnabled() const {
        return true;
    }

    ProjectExplorer::Abi Qabi() const {
        ProjectExplorer::Abi hostAbi = ProjectExplorer::Abi::hostAbi();
        return ProjectExplorer::Abi(hostAbi.architecture(), hostAbi.os(), hostAbi.osFlavor(),
                                    ProjectExplorer::Abi::RuntimeQmlFormat, hostAbi.wordWidth());
    }

    ~CRunConfiguration() {}
};

class CRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory {
    Q_OBJECT

public:
    explicit CRunConfigurationFactory() {
        setObjectName(QLatin1String("CRunConfigurationFactory"));
    }

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

private:
    bool canHandle(ProjectExplorer::Target *parent) const;
};


}

#endif
