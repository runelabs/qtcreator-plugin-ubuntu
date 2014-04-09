#include "ubuntuprojectguesser.h"

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

const char SCOPES_TYPE_CACHE_PROPERTY[]    = "__ubuntu_sdk_is_scopes_project_property";
const char SCOPES_INIFILE_CACHE_PROPERTY[] = "__ubuntu_sdk_scopes_project_inifile_property";
const char CLICK_TYPE_CACHE_PROPERTY[]     = "__ubuntu_sdk_is_click_project_property";

UbuntuProjectGuesser::UbuntuProjectGuesser()
{
}

bool UbuntuProjectGuesser::isScopesProject(ProjectExplorer::Project *project, QString *iniFileName)
{
    //query the project type from CMakeCache always, only fall back on searching the directory
    QString type = projectTypeFromCacheOrProject(project);
    qDebug()<<"Project type from CMake "<<type;
    if (!type.isEmpty()) {
        return QString::compare(type, QLatin1String("Scope") , Qt::CaseInsensitive) == 0;
    }

    QVariant cachedResult = project->property(SCOPES_TYPE_CACHE_PROPERTY);
    if(cachedResult.isValid()) {
        if(!cachedResult.toBool())
            return false;

        if(iniFileName) {
            QVariant cachedIni = project->property(SCOPES_INIFILE_CACHE_PROPERTY);
            *iniFileName = cachedIni.toString();
        }
        return true;
    }

    if(!qobject_cast<CMakeProjectManager::CMakeProject*>(project))
        return false;

    Utils::FileName iniFile = findScopesIniRecursive(Utils::FileName::fromString(project->projectDirectory()));
    QFileInfo info = iniFile.toFileInfo();
    if (iniFileName && info.exists()) {
        *iniFileName = info.absolutePath();
    }

    if(info.exists()){
        project->setProperty(SCOPES_TYPE_CACHE_PROPERTY,true);
        project->setProperty(SCOPES_INIFILE_CACHE_PROPERTY,iniFile.toString());
    }

    return info.exists();
}

bool UbuntuProjectGuesser::isClickAppProject(ProjectExplorer::Project *project)
{
    //query the project type from CMakeCache always, only fall back on searching the directory
    QString type = projectTypeFromCacheOrProject(project);
    qDebug()<<"Project type from CMake "<<type;
    if (!type.isEmpty()) {
        return QString::compare(type, QLatin1String("ClickApp") , Qt::CaseInsensitive) == 0;
    }

    QVariant cachedResult = project->property(CLICK_TYPE_CACHE_PROPERTY);
    if(cachedResult.isValid()) {
        return cachedResult.toBool();
    }

    if(!qobject_cast<CMakeProjectManager::CMakeProject*>(project))
        return false;

    QFile projectFile(project->projectFilePath());
    if (!projectFile.open(QIODevice::ReadOnly))
        return false;

    bool usesClick = false;
    QRegularExpression regExp(QLatin1String("include\\(Click\\)|CLICK_MODE"));
    QTextStream in(&projectFile);
    while (!in.atEnd()) {
        QString contents = in.readLine();
        QRegularExpressionMatch m = regExp.match(contents);
        if(m.hasMatch()) {
            usesClick = true;
            break;
        }
    }

    if(usesClick) {
        Utils::FileName iniFile = findFileRecursive(Utils::FileName::fromString(project->projectDirectory()),
                                                     QLatin1String("^.*desktop.in.*$"));
        QFileInfo info = iniFile.toFileInfo();
        if(info.exists()) {
            project->setProperty(CLICK_TYPE_CACHE_PROPERTY,true);
            return true;
        }

        iniFile = findFileRecursive(Utils::FileName::fromString(project->projectDirectory()),
                                                     QLatin1String("^.*desktop.*$"));
        info = iniFile.toFileInfo();
        if(info.exists()) {
            project->setProperty(CLICK_TYPE_CACHE_PROPERTY,true);
            return true;
        }
    }

    project->setProperty(CLICK_TYPE_CACHE_PROPERTY,false);
    return false;
}

Utils::FileName UbuntuProjectGuesser::findScopesIniRecursive(const Utils::FileName &searchdir)
{
    return findFileRecursive(searchdir,QLatin1String("^.*-scope.ini.*$"));
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

QString UbuntuProjectGuesser::projectTypeFromCacheOrProject(ProjectExplorer::Project *project)
{
    //First try to get the variable from the Cache file
    if(project->activeTarget()
            && project->activeTarget()->activeBuildConfiguration())
    {
        QFile cache(project->activeTarget()->activeBuildConfiguration()->buildDirectory().toString()
                    + QDir::separator()
                    + QLatin1String("CMakeCache.txt"));

        if(cache.exists() && cache.open(QIODevice::ReadOnly)) {
            QRegularExpression regExp(QLatin1String("^UBUNTU_PROJECT_TYPE:(.*)=\\s*(\\S*)\\s*$"));
            QTextStream in(&cache);
            while (!in.atEnd()) {
                QString contents = in.readLine();
                QRegularExpressionMatch m = regExp.match(contents);
                if(m.hasMatch()) {
                    return m.captured(2);
                }
            }
        }
    }

    QFile projectFile(project->projectFilePath());
    if (!projectFile.exists() || !projectFile.open(QIODevice::ReadOnly)) {
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
