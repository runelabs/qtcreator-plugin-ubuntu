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

#ifndef CPROJECT_H
#define CPROJECT_H

#include "global.h"
#include "constants.h"
#include "cprojectfile.h"
#include "common.h"

namespace CordovaUbuntuProjectManager {

class CProjectManager;
class CProjectFile;
class CProjectNode;
class CProject: public ProjectExplorer::Project {
    Q_OBJECT
public:
    CProject(CProjectManager *manager, const QString &fileName);

    QString displayName() const;
    Core::Id id() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;

    ProjectExplorer::ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

    QDir projectDir() const {
        return QFileInfo(document()->fileName()).dir();
    }

    QString filesFileName() const {
        return m_fileName;
    }

private:
    CProjectManager *m_manager;
    QString m_projectName;
    QSharedPointer<CProjectFile> m_file;

    QString m_fileName;
    QSharedPointer<CProjectNode> m_rootNode;
};

}
#endif
