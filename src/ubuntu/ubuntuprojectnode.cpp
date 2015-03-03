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

#include "ubuntuprojectnode.h"

using namespace Ubuntu::Internal;

UbuntuProjectNode::UbuntuProjectNode(UbuntuProject *project, Core::IDocument *projectFile)
    : ProjectExplorer::ProjectNode(projectFile->filePath()),
      m_project(project),
      m_projectFile(projectFile) {
    setDisplayName(projectFile->filePath().toFileInfo().completeBaseName());
    refresh();
}

Core::IDocument *UbuntuProjectNode::projectFile() const {
    return m_projectFile;
}

QString UbuntuProjectNode::projectFilePath() const {
    return m_projectFile->filePath().toString();
}

void UbuntuProjectNode::refresh() {
    using namespace ProjectExplorer;

    removeFileNodes(fileNodes());
    removeFolderNodes(subFolderNodes());

    QStringList files = m_project->files(Project::AllFiles);
    files.removeAll(m_project->filesFileName());

    QHash<QString, QStringList> filesInDirectory;

    foreach (const QString &fileName, files) {
        QFileInfo fileInfo(fileName);

        QString absoluteFilePath;
        QString relativeDirectory;

        if (fileInfo.isAbsolute()) {
            absoluteFilePath = fileInfo.filePath();
            relativeDirectory = m_project->projectDir().relativeFilePath(fileInfo.path());
        } else {
            absoluteFilePath = m_project->projectDir().absoluteFilePath(fileInfo.filePath());
            relativeDirectory = fileInfo.path();
            if (relativeDirectory == QLatin1String("."))
                relativeDirectory.clear();
        }

        filesInDirectory[relativeDirectory].append(absoluteFilePath);
    }

    const QHash<QString, QStringList>::ConstIterator cend = filesInDirectory.constEnd();
    for (QHash<QString, QStringList>::ConstIterator it = filesInDirectory.constBegin(); it != cend; ++it) {
        FolderNode *folder = findOrCreateFolderByName(it.key());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value()) {
            FileType fileType = SourceType; // ### FIXME
            FileNode *fileNode = new FileNode(Utils::FileName::fromString(file), fileType, false);
            fileNodes.append(fileNode);
        }

        folder->addFileNodes(fileNodes);
    }

    m_folderByName.clear();
}

ProjectExplorer::FolderNode *UbuntuProjectNode::findOrCreateFolderByName(const QStringList &components, int end) {
    if (! end)
        return 0;

    QString baseDir = path().toFileInfo().path();

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }

    const QString component = components.at(end - 1);

    if (component.isEmpty())
        return this;

    else if (FolderNode *folder = m_folderByName.value(folderName))
        return folder;

    FolderNode *folder = new FolderNode(Utils::FileName::fromString(baseDir + QLatin1Char('/') + folderName));
    folder->setDisplayName(component);

    m_folderByName.insert(folderName, folder);

    FolderNode *parent = findOrCreateFolderByName(components, end - 1);
    if (! parent)
        parent = this;

    parent->addFolderNodes(QList<FolderNode*>() << folder);

    return folder;
}

ProjectExplorer::FolderNode *UbuntuProjectNode::findOrCreateFolderByName(const QString &filePath) {
    QStringList components = filePath.split(QLatin1Char('/'));
    return findOrCreateFolderByName(components, components.length());
}

QList<ProjectExplorer::ProjectAction> UbuntuProjectNode::supportedActions(Node *node) const {
    Q_UNUSED(node);
    QList<ProjectExplorer::ProjectAction> actions;
    actions.append(ProjectExplorer::AddNewFile);
    actions.append(ProjectExplorer::EraseFile);
    actions.append(ProjectExplorer::Rename);
    return actions;
}

bool UbuntuProjectNode::canAddSubProject(const QString &proFilePath) const {
    Q_UNUSED(proFilePath)
    return false;
}

bool UbuntuProjectNode::addSubProjects(const QStringList &proFilePaths) {
    Q_UNUSED(proFilePaths)
    return false;
}

bool UbuntuProjectNode::removeSubProjects(const QStringList &proFilePaths) {
    Q_UNUSED(proFilePaths)
    return false;
}

bool UbuntuProjectNode::addFiles(const QStringList &, QStringList *) {
    return false;
}

bool UbuntuProjectNode::removeFiles(const QStringList &, QStringList *) {
    return false;
}

bool UbuntuProjectNode::deleteFiles(const QStringList &) {
    return true;
}

bool UbuntuProjectNode::renameFile(const QString &, const QString &) {
    return true;
}

QList<ProjectExplorer::RunConfiguration *> UbuntuProjectNode::runConfigurations( ) const {
    return QList<ProjectExplorer::RunConfiguration *>();
}
