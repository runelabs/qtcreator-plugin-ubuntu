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

#ifndef UBUNTUPROJECTNODE_H
#define UBUNTUPROJECTNODE_H

#include <QObject>

#include "ubuntuproject.h"
#include "ubuntuprojectmanager.h"

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

class UbuntuProject;
class UbuntuProjectNode : public ProjectExplorer::ProjectNode
{
public:
    UbuntuProjectNode(UbuntuProject *project, Core::IDocument *projectFile);

    Core::IDocument *projectFile() const;
    QString projectFilePath() const;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;

    bool canAddSubProject(const QString &proFilePath) const override;

    bool addSubProjects(const QStringList &proFilePaths) override;
    bool removeSubProjects(const QStringList &proFilePaths) override;

    bool addFiles(const QStringList &filePaths,
                          QStringList *notAdded = 0) override;

    bool removeFiles(const QStringList &filePaths,
                             QStringList *notRemoved = 0) override;

    bool deleteFiles(const QStringList &filePaths) override;

    bool renameFile(const QString &filePath,
                            const QString &newFilePath) override;
    QList<ProjectExplorer::RunConfiguration *> runConfigurations( ) const override;


    void refresh();

private:
    FolderNode *findOrCreateFolderByName(const QString &filePath);
    FolderNode *findOrCreateFolderByName(const QStringList &components, int end);

private:
    UbuntuProject *m_project;
    Core::IDocument *m_projectFile;
    QHash<QString, FolderNode *> m_folderByName;
    
};
}
}

#endif // UBUNTUPROJECTNODE_H
