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

#include "cproject.h"
#include "cprojectmanager.h"
#include "cruncontrol.h"
#include "cprojectnode.h"

namespace CordovaUbuntuProjectManager {

CProject::CProject(CProjectManager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName) {
    setProjectContext(Core::Context(PROJECTCONTEXT));

    QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.completeBaseName();

    m_file = QSharedPointer<CProjectFile>(new CProjectFile(this, fileName));

    Core::DocumentManager::addDocument(m_file.data(), true);

    m_rootNode = QSharedPointer<CProjectNode>(new CProjectNode(this, m_file.data()));
    m_manager->registerProject(this);

    QList<ProjectExplorer::Kit *> kits = ProjectExplorer::KitManager::instance()->kits();
    foreach (ProjectExplorer::Kit *kit, kits) {
        addTarget(createTarget(kit));
    }

}

QString CProject::displayName() const {
    return m_projectName;
}

Core::Id CProject::id() const {
    return Core::Id("CordovaUbuntuProjectManager.CordovaUbuntuProject");
}

Core::IDocument *CProject::document() const {
    return m_file.data();
}

ProjectExplorer::IProjectManager *CProject::projectManager() const {
    return m_manager;
}

ProjectExplorer::ProjectNode *CProject::rootProjectNode() const {
    return m_rootNode.data();
}

static void enumChild(const QDir &dir, QStringList &res) {
    foreach (const QFileInfo &info, dir.entryInfoList(QDir::NoDotAndDotDot|QDir::Dirs|QDir::Files)) {
        if (info.fileName().indexOf(QLatin1String(".cordovaproject")) != -1)
            continue;
        if (info.isFile()) {
            res.append(info.absoluteFilePath());
        } else if (info.isDir()) {
            enumChild(QDir(info.absoluteFilePath()), res);
        }
    }
}

QStringList CProject::files(FilesMode) const {
    QStringList files;
    enumChild(projectDir(), files);
    return files;
}

}
