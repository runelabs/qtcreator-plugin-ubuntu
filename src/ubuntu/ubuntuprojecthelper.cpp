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

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>

#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <cmakeprojectmanager/cmakebuildconfiguration.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <QDebug>

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>

#include <glib-2.0/glib.h>


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

        CMakeProjectManager::CMakeConfig fullCache = CMakeProjectManager::CMakeProject::activeCmakeCacheForTarget(target);

        QString manifestPath;

        QByteArray fromCache = CMakeProjectManager::CMakeConfigItem::valueOf("UBUNTU_MANIFEST_PATH", fullCache );
        if (!fromCache.isEmpty()) {
            manifestPath = QString::fromUtf8(fromCache);
        } else {
            manifestPath = defaultValue;
        }

        if(QDir::isAbsolutePath(manifestPath))
            return manifestPath;

        Utils::FileName projectDir = target->project()->projectDirectory();
        return projectDir.appendPath(manifestPath).toString();

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

/*!
 * \brief UbuntuProjectHelper::injectScopeDebugHelper
 * Injects the \a commandTemplate into the scope ini.
 * Replaces %S with \a scriptName and %C with the subcommand.
 * If no ScopeRunner is set \a defaultSubCmd is used.
 */
bool UbuntuProjectHelper::injectScopeDebugHelper(const QString &iniFilePath, const QString &scriptName, const QString &commandTemplate, const QString &defaultSubCmd)
{
    GKeyFile* keyFile = g_key_file_new();
    GKeyFileFlags flags = static_cast<GKeyFileFlags>(G_KEY_FILE_KEEP_TRANSLATIONS|G_KEY_FILE_KEEP_COMMENTS);
    if(!g_key_file_load_from_file(keyFile,qPrintable(iniFilePath),flags,NULL)){
        g_key_file_free(keyFile);
        qWarning()<<"Could not read the ini file";
        return false;
    }

    QString subCmd;
    if(g_key_file_has_key(keyFile,"ScopeConfig","ScopeRunner",NULL)) {
        gchar *value = g_key_file_get_string(keyFile,"ScopeConfig","ScopeRunner",NULL);
        if(value == NULL) {
            qWarning()<<"Could not read the ScopeRunner entry";
            g_key_file_free(keyFile);
            return false;
        }

        subCmd = QString::fromUtf8(value);
        g_free(value);

    } else {
        subCmd = defaultSubCmd;
    }

    if (!subCmd.contains(scriptName)) {
        QString command = QString(commandTemplate)
                .replace(QStringLiteral("%S"),scriptName)
                .replace(QStringLiteral("%C"),subCmd);
        g_key_file_set_string(keyFile,"ScopeConfig","ScopeRunner",command.toUtf8().data());
    }

    g_key_file_set_boolean(keyFile,"ScopeConfig","DebugMode",TRUE);

    gsize size = 0;
    gchar *settingData = g_key_file_to_data (keyFile, &size, NULL);
    if(!settingData) {
        qWarning()<<"Could not convert the new data into the ini file";
        g_key_file_free(keyFile);
        return false;
    }

    gboolean ret = g_file_set_contents (qPrintable(iniFilePath), settingData, size,  NULL);
    g_free (settingData);
    g_key_file_free (keyFile);

    return ret == TRUE;
}

QString UbuntuProjectHelper::getManifestPath(ProjectExplorer::Project *p, const QString &defaultValue)
{
    if(!p)
        return defaultValue;

    return getManifestPath(p->activeTarget(),defaultValue);
}

} // namespace Internal
} // namespace Ubuntu
