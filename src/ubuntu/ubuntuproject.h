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

#ifndef UBUNTUPROJECT_H
#define UBUNTUPROJECT_H

#include "ubuntuprojectmanager.h"
#include "ubuntuprojectfile.h"
#include "ubuntuprojectnode.h"
#include "ubuntuconstants.h"
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
#include <projectexplorer/buildconfiguration.h>

namespace Ubuntu {
namespace Internal {

class UbuntuProjectManager;
class UbuntuProjectFile;
class UbuntuProjectNode;

class UbuntuKitMatcher : public ProjectExplorer::KitMatcher
{
public:
    explicit UbuntuKitMatcher();
    static bool matches(const ProjectExplorer::Kit *k);
};

class UbuntuProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    UbuntuProject(UbuntuProjectManager *manager, const QString &fileName);

    QString displayName() const override;
    ProjectExplorer::IProjectManager *projectManager() const override;

    ProjectExplorer::ProjectNode *rootProjectNode() const override;
    QStringList files(FilesMode fileMode) const override;

    QDir projectDir() const {
        return projectDirectory().toString();
    }

    QString filesFileName() const {
        return m_fileName;
    }

    QString mainFile() const {
        return m_mainFile;
    }

    // Project interface
    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const override;
    bool needsConfiguration() const override;
    bool requiresTargetPanel() const override;

    static QString shadowBuildDirectory(const QString &proFilePath, const ProjectExplorer::Kit *k,
                                        const QString &suffix = QString(),
                                        const ProjectExplorer::BuildConfiguration::BuildType buildType = ProjectExplorer::BuildConfiguration::Unknown);
private:
    void extractProjectFileData(const QString& filename);

    UbuntuProjectManager *m_manager;
    QString m_projectName;
    QSharedPointer<UbuntuProjectFile> m_file;
    QString m_mainFile;

    QString m_fileName;
    QSharedPointer<UbuntuProjectNode> m_rootNode;
};
}
}

#endif // UBUNTUPROJECT_H
