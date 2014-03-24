#include "ubunturemoterunconfiguration.h"
#include "clicktoolchain.h"
#include "ubuntuclicktool.h"
#include "ubuntucmakebuildconfiguration.h"
#include "ubuntuclickmanifest.h"
#include "ubuntuconstants.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>
#include <utils/qtcprocess.h>
#include <cmakeprojectmanager/cmakeproject.h>

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QVariantMap>
#include <QVariant>
#include <QFileInfo>

namespace Ubuntu {
namespace Internal {

UbuntuRemoteRunConfiguration::UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent)
    : AbstractRemoteLinuxRunConfiguration(parent,typeId())
{
    setDisplayName(parent->project()->displayName());
    addExtraAspect(new RemoteLinux::RemoteLinuxEnvironmentAspect(this));
}

UbuntuRemoteRunConfiguration::UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent, UbuntuRemoteRunConfiguration *source)
    : AbstractRemoteLinuxRunConfiguration(parent,source)
{
}

QString UbuntuRemoteRunConfiguration::localExecutableFilePath() const
{
    return m_localExecutable;
}

QString UbuntuRemoteRunConfiguration::remoteExecutableFilePath() const
{
    return m_remoteExecutable;
}

QStringList UbuntuRemoteRunConfiguration::arguments() const
{
    return m_arguments;
}

QString UbuntuRemoteRunConfiguration::workingDirectory() const
{
    return QString::fromLatin1("/home/phablet/dev_tmp/%1").arg(target()->project()->displayName());
}

QString UbuntuRemoteRunConfiguration::alternateRemoteExecutable() const
{
    return QString();
}

bool UbuntuRemoteRunConfiguration::useAlternateExecutable() const
{
    return false;
}

Utils::Environment UbuntuRemoteRunConfiguration::environment() const
{
    RemoteLinux::RemoteLinuxEnvironmentAspect *aspect = extraAspect<RemoteLinux::RemoteLinuxEnvironmentAspect>();
    QTC_ASSERT(aspect, return Utils::Environment());
    Utils::Environment env(Utils::OsTypeLinux);
    env.modify(aspect->userEnvironmentChanges());
    env.appendOrSet(QLatin1String("gnutriplet"),QLatin1String("arm-linux-gnueabihf"));
    env.appendOrSet(QLatin1String("pkgdir"),workingDirectory());
    env.appendOrSet(QLatin1String("APP_ID"),m_appId);
    return env;
}

QStringList UbuntuRemoteRunConfiguration::soLibSearchPaths() const
{
    QStringList paths;
    CMakeProjectManager::CMakeProject *cmakeProj
            = qobject_cast<CMakeProjectManager::CMakeProject *>(target()->project());

    if(cmakeProj) {
        QList<CMakeProjectManager::CMakeBuildTarget> targets = cmakeProj->buildTargets();
        foreach(const CMakeProjectManager::CMakeBuildTarget& target, targets) {
            QFileInfo binary(target.executable);
            if(binary.exists()) {
                qDebug()<<"Adding path "<<binary.absolutePath()<<" to solib-search-paths";
                paths << binary.absolutePath();
            }
        }
    }
    return paths;
}

QWidget *UbuntuRemoteRunConfiguration::createConfigurationWidget()
{
    return 0;
}

bool UbuntuRemoteRunConfiguration::isEnabled() const
{
    return true;
}

QString UbuntuRemoteRunConfiguration::disabledReason() const
{
    return QString();
}

bool UbuntuRemoteRunConfiguration::isConfigured() const
{
    return true;
}

/*!
 * \brief UbuntuRemoteRunConfiguration::ensureConfigured
 * Configures the internal parameters and fetched the informations from
 * the manifest file. We have no way of knowing these things before the project
 * was build and all files are in <builddir>/package. That means we can not cache that
 * information, because it could change anytime
 */
bool UbuntuRemoteRunConfiguration::ensureConfigured(QString *errorMessage)
{
    qDebug()<<"--------------------- Reconfiguring RunConfiguration ----------------------------";
    Utils::FileName buildDir = target()->activeBuildConfiguration()->buildDirectory();
    QDir package_dir(buildDir.toString()+QDir::separator()+QLatin1String("package"));
    if(!package_dir.exists()) {
        if(errorMessage)
            *errorMessage = tr("No packaging directory available, please check if the deploy configuration is correct.");

        return false;
    }

    //read the manifest
    UbuntuClickManifest manifest;
    if(!manifest.load(package_dir.absoluteFilePath(QLatin1String("manifest.json")),target()->project()->displayName())) {
        if(errorMessage)
            *errorMessage = tr("Could not open the manifest file in the package directory, maybe you have to create one.");

        return false;
    }

    //get the hooks property that contains the desktop file
    QScriptValue hooks = manifest.hooks();
    if(hooks.isNull()) {
        if(errorMessage)
            *errorMessage = tr("No hooks property in the manifest");
        return false;
    }

    //check if the hooks property is a object
    QVariantMap hookMap = hooks.toVariant().toMap();
    if(hookMap.isEmpty()){
        if(errorMessage)
            *errorMessage = tr("Hooks needs to be a object");
        return false;
    }

    //get the app id and check if the first property is a object
    m_appId = hookMap.firstKey();
    QVariant app = hookMap.first();
    if(!app.type() == QVariant::Map) {
        if(errorMessage)
            *errorMessage = tr("The hooks property can only contain objects");
        return false;
    }

    //get the desktop file path in the package
    QVariantMap hook = app.toMap();
    if(!hook.contains(QLatin1String("desktop"))) {
        if(errorMessage)
            *errorMessage = tr("No desktop file found in hook: %1").arg(m_appId);
        return false;
    }

    m_desktopFile = QDir::cleanPath(package_dir.absoluteFilePath(hook[QLatin1String("desktop")].toString()));
    if(!readDesktopFile(errorMessage))
        return false;

    m_arguments.append(QString::fromLatin1("--desktop_file_hint=/home/phablet/dev_tmp/%1/%2")
                       .arg(target()->project()->displayName())
                       .arg(hook[QLatin1String("desktop")].toString()));
    m_arguments.append(QLatin1String("--stage_hint=main_stage"));
    return true;
}

bool UbuntuRemoteRunConfiguration::fromMap(const QVariantMap &map)
{
    qDebug()<<Q_FUNC_INFO;
    return AbstractRemoteLinuxRunConfiguration::fromMap(map);
}

QVariantMap UbuntuRemoteRunConfiguration::toMap() const
{
    QVariantMap m = AbstractRemoteLinuxRunConfiguration::toMap();
    qDebug()<<Q_FUNC_INFO;
    return m;
}

Core::Id UbuntuRemoteRunConfiguration::typeId()
{
    return Core::Id("UbuntuProjectManager.RemoteRunConfiguration");
}

/*!
 * \brief UbuntuRemoteRunConfiguration::readDesktopFile
 * Tries to read the Exec line from the desktop file, to
 * extract arguments and to know which "executor" is used on
 * the phone
 */
bool UbuntuRemoteRunConfiguration::readDesktopFile(QString *errorMessage)
{
    QFile desktop(m_desktopFile);
    if(!desktop.exists()) {
        if(errorMessage)
            *errorMessage = tr("Desktop file does not exist");
        return false;
    }
    if(!desktop.open(QIODevice::ReadOnly)) {
        if(errorMessage)
            *errorMessage = tr("Could not open desktop file for reading");
        return false;
    }

    QString execLine;
    QString name;

    QTextStream in(&desktop);
    while(!in.atEnd()) {
        QString line = in.readLine();
        if(line.startsWith(QLatin1String("#")))
            continue;

        line = line.mid(0,line.indexOf(QChar::fromLatin1('#'))).trimmed();
        if(line.startsWith(QLatin1String("Exec"),Qt::CaseInsensitive)) {
            execLine = line.mid(line.indexOf(QChar::fromLatin1('='))+1);
            qDebug()<<"Found exec line: "<<execLine;
            continue;
        } else if(line.startsWith(QLatin1String("Name"),Qt::CaseInsensitive)) {
            name = line.mid(line.indexOf(QChar::fromLatin1('='))+1);
            qDebug()<<"Found name line: "<<name;
            continue;
        }
    }

    if(execLine.startsWith(QLatin1String("qmlscene"))) {
        ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
        if(tc->type() != QString::fromLatin1(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
            if(errorMessage)
                *errorMessage = tr("Wrong toolchain type. Please check your build configuration.");
            return false;
        }

        ClickToolChain* clickTc = static_cast<ClickToolChain*>(tc);
        m_localExecutable  = QString::fromLatin1("%1/usr/lib/arm-linux-gnueabihf/qt5/bin/qmlscene")
                .arg(UbuntuClickTool::targetBasePath(clickTc->clickTarget()));
        m_remoteExecutable = QLatin1String("/usr/bin/qmlscene");

        execLine.replace(QLatin1String("qmlscene"),QString());
        m_arguments = Utils::QtcProcess::splitArgs(execLine);
    }
    return true;
}

} // namespace Internal
} // namespace Ubuntu
