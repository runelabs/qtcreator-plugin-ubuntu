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
#include "ubuntuprojecthelper.h"
#include "ubuntucmakecache.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>

#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <QDebug>

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>


namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuProjectHelper::UbuntuProjectHelper()
{
}

Utils::FileName UbuntuProjectHelper::findScopesIniRecursive(const Utils::FileName &searchdir, const QString &appid)
{
    return findFileRecursive(searchdir,QStringLiteral("^.*_%1\\.ini.*$").arg(appid));
}

Utils::FileName UbuntuProjectHelper::findFileRecursive(const Utils::FileName &searchdir, const QString &regexp)
{
    QRegularExpression regex(regexp);
    return findFileRecursive(searchdir,regex);
}

Utils::FileName UbuntuProjectHelper::findFileRecursive(const Utils::FileName &searchdir, const QRegularExpression &regexp)
{
    QFileInfo dirInfo = searchdir.toFileInfo();
    if(!dirInfo.exists())
        return Utils::FileName();

    if(!dirInfo.isDir())
        return Utils::FileName();

    QDir dir(dirInfo.absoluteFilePath());
    QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden);

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

QList<Utils::FileName> UbuntuProjectHelper::findFilesRecursive(const Utils::FileName &searchdir, const QRegularExpression &regexp)
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

QString UbuntuProjectHelper::getManifestPath(ProjectExplorer::Target *target, const QString &defaultValue)
{
    if(target && target->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID ) {
        QVariant manifestPath = UbuntuCMakeCache::getValue(QStringLiteral("UBUNTU_MANIFEST_PATH"),
                                                           target->activeBuildConfiguration(),
                                                           defaultValue);

        Utils::FileName projectDir = target->project()->projectDirectory();
        return projectDir.appendPath(manifestPath.toString()).toString();
    } else if (target && target->project()->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID ) {
        QmakeProjectManager::QmakeProject *qmakeProj = static_cast<QmakeProjectManager::QmakeProject *>(target->project());
        QList<QmakeProjectManager::QmakeProFileNode *> nodes = qmakeProj->allProFiles();

        QString manifestFilePath; //empty
        foreach (QmakeProjectManager::QmakeProFileNode *node, nodes) {
            if(!node)
                continue;

            manifestFilePath = node->singleVariableValue(QmakeProjectManager::UbuntuManifestFile);
            if(manifestFilePath.isEmpty()) {
                continue;
            } else if(QDir::isRelativePath(manifestFilePath)) {
                manifestFilePath = QDir::cleanPath(node->sourceDir()
                                                   +QDir::separator()
                                                   +manifestFilePath);
                break;
            } else {
                manifestFilePath = QDir::cleanPath(manifestFilePath);
                break;
            }
        }

        if(manifestFilePath.isEmpty())
            return defaultValue;
        return manifestFilePath;
    }
    return defaultValue;
}

QString UbuntuProjectHelper::getManifestPath(ProjectExplorer::Project *p, const QString &defaultValue)
{
    if(!p)
        return defaultValue;

    return getManifestPath(p->activeTarget(),defaultValue);
}

} // namespace Internal
} // namespace Ubuntu
