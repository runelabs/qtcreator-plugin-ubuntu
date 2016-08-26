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

#include "ubuntuprojectfile.h"

using namespace Ubuntu::Internal;

UbuntuProjectFile::UbuntuProjectFile(UbuntuProject *parent, QString fileName)
    : Core::IDocument(parent),
      m_project(parent),
      m_fileName(fileName) {
    QTC_CHECK(m_project);
    QTC_CHECK(!fileName.isEmpty());
    setFilePath(Utils::FileName::fromString(fileName));
    setMimeType(QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE));
}

bool UbuntuProjectFile::save(QString *, const QString &, bool) {
    return false;
}

bool UbuntuProjectFile::isModified() const {
    return false;
}

bool UbuntuProjectFile::isSaveAsAllowed() const {
    return false;
}

Core::IDocument::ReloadBehavior UbuntuProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const {
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool UbuntuProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType) {
    Q_UNUSED(errorString)
    Q_UNUSED(flag)

    return true;
}
