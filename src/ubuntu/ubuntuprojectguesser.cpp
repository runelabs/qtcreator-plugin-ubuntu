#include "ubuntuprojectguesser.h"

#include <projectexplorer/project.h>
#include <QDebug>

namespace Ubuntu {
namespace Internal {

const char SCOPES_TYPE_CACHE_PROPERTY[] = "__ubuntu_sdk_is_scopes_project_property";

UbuntuProjectGuesser::UbuntuProjectGuesser()
{
}

bool UbuntuProjectGuesser::isScopesProject(ProjectExplorer::Project *project, QString *iniFileName)
{
    QVariant cachedResult = project->property(SCOPES_TYPE_CACHE_PROPERTY);
    if(cachedResult.isValid()) {
        if(iniFileName)
            *iniFileName = cachedResult.toString();
        return true;
    }

    Utils::FileName iniFile = findScopesIniRecursive(Utils::FileName::fromString(project->projectDirectory()));
    QFileInfo info = iniFile.toFileInfo();
    if (iniFileName && info.exists()) {
        *iniFileName = info.absolutePath();
    }

    qDebug()<<"Project DIr: "<<project->projectDirectory();

    if(info.exists())
        project->setProperty(SCOPES_TYPE_CACHE_PROPERTY,iniFile.toString());

    return info.exists();
}

bool UbuntuProjectGuesser::isClickProject(ProjectExplorer::Project *project)
{
    Utils::FileName iniFile = findFileRecursive(Utils::FileName::fromString(project->projectDirectory()),
                                                 QLatin1String("^.*desktop.in.*$"));
    QFileInfo info = iniFile.toFileInfo();
    return info.exists();
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
    qDebug()<<"Starting in DIr: "<<searchdir.toString();
    QFileInfo dirInfo = searchdir.toFileInfo();
    if(!dirInfo.exists())
        return Utils::FileName();

    if(!dirInfo.isDir())
        return Utils::FileName();

    QDir dir(dirInfo.absoluteFilePath());
    QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

    foreach (const QString& entry, entries) {
        qDebug()<<"Looking at file: "<<dir.absoluteFilePath(entry)<<" "<<entry;
        QFileInfo info(dir.absoluteFilePath(entry));
        if(info.isDir()) {
            qDebug()<<"Is Dir";
            Utils::FileName f = findFileRecursive(Utils::FileName::fromString(dir.absoluteFilePath(entry)),regexp);
            if(!f.isEmpty())
                return f;

            continue;
        }

        QRegularExpressionMatch match = regexp.match(entry);
        if(match.hasMatch()) {
            qDebug()<<"Found";
            return Utils::FileName(info);
        }
    }

    return Utils::FileName();
}

} // namespace Internal
} // namespace Ubuntu
