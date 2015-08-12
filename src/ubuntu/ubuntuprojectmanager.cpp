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

#include "ubuntuprojectmanager.h"
#include <QDebug>
#include <qmlprojectmanager/qmlprojectmanager.h>

using namespace Ubuntu::Internal;

UbuntuProjectManager::UbuntuProjectManager() {
    QObject* sessionManager = ProjectExplorer::SessionManager::instance();

    connect(sessionManager,SIGNAL(projectAdded(ProjectExplorer::Project*)),SLOT(onProjectAdded(ProjectExplorer::Project*)));
}

ProjectExplorer::Project* UbuntuProjectManager::openProject(const QString &filePath, QString *errorString) {
    QFileInfo fileInfo(filePath);

    foreach (ProjectExplorer::Project *pi, ProjectExplorer::SessionManager::projects()) {
        if (filePath == pi->document()->filePath().toString()) {
            if (errorString)
                *errorString = tr("Failed opening project '%1': Project already open") .arg(QDir::toNativeSeparators(filePath));
            return 0;
        }
    }

    if (fileInfo.isFile())
        return new UbuntuProject(this, filePath);

    *errorString = tr("Failed opening project '%1': Project file is not a file").arg(QDir::toNativeSeparators(filePath));
    return 0;
}

void UbuntuProjectManager::registerProject(UbuntuProject *project) {
    m_projects.append(project);
}

void UbuntuProjectManager::unregisterProject(UbuntuProject *project) {
    m_projects.removeAll(project);
}

QString UbuntuProjectManager::mimeType() const {
    return QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE);
}

void UbuntuProjectManager::onProjectAdded(ProjectExplorer::Project* addedProject) {
    QString mimeType = addedProject->projectManager()->mimeType();
    qDebug() << mimeType;
}
