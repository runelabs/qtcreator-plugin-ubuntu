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
#include "ubuntuprojectguesser.h"
#include "ubuntucmakecache.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <QDebug>

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>


namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

const char SCOPES_TYPE_CACHE_PROPERTY[]    = "__ubuntu_sdk_is_scopes_project_property";
const char SCOPES_INIFILE_CACHE_PROPERTY[] = "__ubuntu_sdk_scopes_project_inifile_property";
const char CLICK_TYPE_CACHE_PROPERTY[]     = "__ubuntu_sdk_is_click_project_property";

UbuntuProjectGuesser::UbuntuProjectGuesser()
{
}

Utils::FileName UbuntuProjectGuesser::findScopesIniRecursive(const Utils::FileName &searchdir, const QString &appid)
{
    return findFileRecursive(searchdir,QStringLiteral("^.*_%1\\.ini.*$").arg(appid));
}

Utils::FileName UbuntuProjectGuesser::findFileRecursive(const Utils::FileName &searchdir, const QString &regexp)
{
    QRegularExpression regex(regexp);
    return findFileRecursive(searchdir,regex);
}

Utils::FileName UbuntuProjectGuesser::findFileRecursive(const Utils::FileName &searchdir, const QRegularExpression &regexp)
{
    QFileInfo dirInfo = searchdir.toFileInfo();
    if(!dirInfo.exists())
        return Utils::FileName();

    if(!dirInfo.isDir())
        return Utils::FileName();

    QDir dir(dirInfo.absoluteFilePath());
    QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

    foreach (const QString& entry, entries) {
        QFileInfo info(dir.absoluteFilePath(entry));
        if(info.isDir()) {
            Utils::FileName f = findFileRecursive(Utils::FileName::fromString(dir.absoluteFilePath(entry)),regexp);
            if(!f.isEmpty())
                return f;

            continue;
        }

        QRegularExpressionMatch match = regexp.match(entry);
        if(match.hasMatch()) {
            return Utils::FileName(info);
        }
    }

    return Utils::FileName();
}

QList<Utils::FileName> UbuntuProjectGuesser::findFilesRecursive(const Utils::FileName &searchdir, const QRegularExpression &regexp)
{
    QList<Utils::FileName> result;
    QFileInfo dirInfo = searchdir.toFileInfo();
    if(!dirInfo.exists())
        return result;

    if(!dirInfo.isDir())
        return result;

    QDir dir(dirInfo.absoluteFilePath());
    QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

    foreach (const QString& entry, entries) {
        QFileInfo info(dir.absoluteFilePath(entry));
        if(info.isDir()) {
            result.append(findFileRecursive(Utils::FileName::fromString(dir.absoluteFilePath(entry)),regexp));
            continue;
        }

        QRegularExpressionMatch match = regexp.match(entry);
        if(match.hasMatch()) {
            result.append(Utils::FileName(info));
        }
    }

    return result;
}

QString UbuntuProjectGuesser::projectTypeFromCacheOrProject(ProjectExplorer::Project *project)
{
    //First try to get the variable from the Cache file
    if(project->activeTarget()
            && project->activeTarget()->activeBuildConfiguration())
    {
        QVariant val = UbuntuCMakeCache::getValue(QStringLiteral("UBUNTU_PROJECT_TYPE"),project->activeTarget()->activeBuildConfiguration());
        if(val.isValid())
            return val.toString();
    }

    QFile projectFile(project->projectFilePath());
    if (projectFile.exists() && projectFile.open(QIODevice::ReadOnly)) {
        QRegularExpression regExp(QLatin1String("^\\s*SET\\s*\\(\\s*UBUNTU_PROJECT_TYPE\\s*\"?(\\S*)\"?"));
        QTextStream in(&projectFile);
        while (!in.atEnd()) {
            QString contents = in.readLine();
            QRegularExpressionMatch m = regExp.match(contents);
            if(m.hasMatch()) {
                return m.captured(1);
            }
        }
    }
    return QString();
}

} // namespace Internal
} // namespace Ubuntu
