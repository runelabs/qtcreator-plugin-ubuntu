/*
 * Copyright 2014 Canonical Ltd.
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
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#ifndef UBUNTU_INTERNAL_UBUNTUPROJECTGUESSER_H
#define UBUNTU_INTERNAL_UBUNTUPROJECTGUESSER_H

#include <utils/fileutils.h>
#include <QRegularExpression>


namespace ProjectExplorer {
    class Project;
    class Target;
}

namespace Ubuntu {
namespace Internal {

class UbuntuProjectHelper
{
public:
    UbuntuProjectHelper();
    static Utils::FileName findScopesIniRecursive (const Utils::FileName &searchdir, const QString &appid);
    static Utils::FileName findFileRecursive (const Utils::FileName &searchdir, const QString &regexp);
    static Utils::FileName findFileRecursive (const Utils::FileName &searchdir, const QRegularExpression &regexp);
    static QList<Utils::FileName> findFilesRecursive(const Utils::FileName &searchdir, const QRegularExpression &regexp);
    static QString getManifestPath (ProjectExplorer::Project * p, const QString &defaultValue);
    static QString getManifestPath(ProjectExplorer::Target *target, const QString &defaultValue);
    static bool injectScopeDebugHelper (const QString &iniFilePath, const QString &scriptName, const QString &commandTemplate, const QString &defaultSubCmd);
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPROJECTGUESSER_H
