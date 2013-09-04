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

#ifndef CPROJECTFILE_H
#define CPROJECTFILE_H

#include "common.h"
#include "cproject.h"

namespace CordovaUbuntuProjectManager {

class CProject;
class CProjectFile: public Core::IDocument {
    Q_OBJECT
public:
    CProjectFile(CProject *parent, QString fileName);
    ~CProjectFile() {}

    bool save(QString *errorString, const QString &fileName, bool autoSave);
    QString fileName() const;
    void rename(const QString &newName);

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType);

private:
    CProject *m_project;
    QString m_fileName;
};

}

#endif // CPROJECTFILE_H
