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

#ifndef CPROJECTNODE_H
#define CPROJECTNODE_H

#include "common.h"

namespace CordovaUbuntuProjectManager {
class CProject;

class CProjectNode : public ProjectExplorer::ProjectNode {
public:
    CProjectNode(CProject *project, Core::IDocument *projectFile);

    Core::IDocument *projectFile() const;
    QString projectFilePath() const;

    virtual bool hasBuildTargets() const;

    virtual QList<ProjectExplorer::ProjectNode::ProjectAction> supportedActions(Node *node) const;

    virtual bool canAddSubProject(const QString &proFilePath) const;

    virtual bool addSubProjects(const QStringList &proFilePaths);
    virtual bool removeSubProjects(const QStringList &proFilePaths);

    virtual bool addFiles(const ProjectExplorer::FileType fileType,
                          const QStringList &filePaths,
                          QStringList *notAdded = 0);

    virtual bool removeFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notRemoved = 0);

    virtual bool deleteFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths);

    virtual bool renameFile(const ProjectExplorer::FileType fileType,
                            const QString &filePath,
                            const QString &newFilePath);
    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);


    void refresh();

private:
    FolderNode *findOrCreateFolderByName(const QString &filePath);
    FolderNode *findOrCreateFolderByName(const QStringList &components, int end);

private:
    CProject *m_project;
    Core::IDocument *m_projectFile;
    QHash<QString, FolderNode *> m_folderByName;
};

}

#endif
