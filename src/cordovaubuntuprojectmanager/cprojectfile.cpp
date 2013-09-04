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

#include "cprojectfile.h"

namespace CordovaUbuntuProjectManager {

CProjectFile::CProjectFile(CProject *parent, QString fileName)
    : Core::IDocument(parent),
      m_project(parent),
      m_fileName(fileName) {
    QTC_CHECK(m_project);
    QTC_CHECK(!fileName.isEmpty());
}

bool CProjectFile::save(QString *, const QString &, bool) {
    return false;
}

void CProjectFile::rename(const QString &newName) {
    // Can't happen...
    Q_UNUSED(newName);
    Q_ASSERT(false);
}

QString CProjectFile::fileName() const {
    return m_fileName;
}

QString CProjectFile::defaultPath() const {
    return QString();
}

QString CProjectFile::suggestedFileName() const {
    return QString();
}

QString CProjectFile::mimeType() const {
    return QLatin1String(CORDOVAUBUNTUPROJECT_MIMETYPE);
}

bool CProjectFile::isModified() const {
    return false;
}

bool CProjectFile::isSaveAsAllowed() const {
    return false;
}

Core::IDocument::ReloadBehavior CProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const {
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool CProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType) {
    Q_UNUSED(errorString)
    Q_UNUSED(flag)

    /*if (type == TypeContents)
        m_project->refreshProjectFile();
*/
    return true;
}

}
