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

#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include "common.h"
#include "cproject.h"

namespace CordovaUbuntuProjectManager {

class CProjectManager: public ProjectExplorer::IProjectManager {
    Q_OBJECT
public:
    CProjectManager() {}

    virtual QString mimeType() const {
        return QLatin1String(CORDOVAUBUNTUPROJECT_MIMETYPE);
    }

    virtual ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString) {
        QFileInfo fileInfo(fileName);
        ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();

        foreach (ProjectExplorer::Project *pi, projectExplorer->session()->projects()) {
            if (fileName == pi->document()->fileName()) {
                if (errorString)
                    *errorString = tr("Failed opening project '%1': Project already open") .arg(QDir::toNativeSeparators(fileName));
                return 0;
            }
        }

        if (fileInfo.isFile())
            return new CProject(this, fileName);

        *errorString = tr("Failed opening project '%1': Project file is not a file").arg(QDir::toNativeSeparators(fileName));
        return 0;
    }
/*
    void notifyChanged(const QString &fileName) {
        foreach (CProject *project, m_projects) {
            if (fileName == project->filesFileName())
                project->refresh(Project::Files);
        }
    }
*/
    void registerProject(CProject *project) {
        m_projects.append(project);
    }

    void unregisterProject(CProject *project) {
        m_projects.removeAll(project);
    }

private:
    QList<CProject*> m_projects;
};

}
#endif
