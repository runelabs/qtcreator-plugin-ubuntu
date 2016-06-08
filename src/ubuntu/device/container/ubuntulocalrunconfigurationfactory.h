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

#ifndef UBUNTURUNCONFIGURATIONFACTORY_H
#define UBUNTURUNCONFIGURATIONFACTORY_H

#include <QtGlobal>
#include <QObject>
#include <ubuntu/ubuntuproject.h>
#include <ubuntu/ubuntuconstants.h>
#include "ubuntulocalrunconfiguration.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/documentmanager.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>

namespace Ubuntu {
namespace Internal {

class UbuntuLocalRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    explicit UbuntuLocalRunConfigurationFactory() {
        setObjectName(QLatin1String("UbuntuRunConfigurationFactory"));
    }

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode = UserCreate) const override;
    QString displayNameForId(const Core::Id id) const override;

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) override;

private:
    bool canHandle(ProjectExplorer::Target *parent) const;
    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, const Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
};

}
}
#endif // UBUNTURUNCONFIGURATIONFACTORY_H
