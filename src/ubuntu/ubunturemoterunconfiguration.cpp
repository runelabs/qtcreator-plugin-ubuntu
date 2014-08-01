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
#include "ubunturemoterunconfiguration.h"
#include "clicktoolchain.h"
#include "ubuntuclicktool.h"
#include "ubuntucmakebuildconfiguration.h"
#include "ubuntuconstants.h"
#include "ubuntulocalrunconfiguration.h"
#include "ubunturemotedeployconfiguration.h"
#include "ubuntupackagestep.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildsteplist.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>
#include <utils/qtcprocess.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QVariantMap>
#include <QVariant>
#include <QFileInfo>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuRemoteRunConfiguration::UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent, Core::Id id)
    : AbstractRemoteLinuxRunConfiguration(parent,id)
{
    setDisplayName(appId());
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
    return QString::fromLatin1("/home/phablet");
}

Utils::Environment UbuntuRemoteRunConfiguration::environment() const
{
    RemoteLinux::RemoteLinuxEnvironmentAspect *aspect = extraAspect<RemoteLinux::RemoteLinuxEnvironmentAspect>();
    QTC_ASSERT(aspect, return Utils::Environment());
    Utils::Environment env(Utils::OsTypeLinux);
    env.modify(aspect->userEnvironmentChanges());
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
                if(debug) qDebug()<<"Adding path "<<binary.absolutePath()<<" to solib-search-paths";
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
    if(debug) qDebug()<<"--------------------- Reconfiguring RunConfiguration ----------------------------";
    m_arguments.clear();
    m_desktopFile.clear();
    m_appId.clear();
    m_clickPackage.clear();

    QDir package_dir(packageDir());
    if(!package_dir.exists()) {
        if(errorMessage)
            *errorMessage = tr("No packaging directory available, please check if the deploy configuration is correct.");

        return false;
    }

    m_desktopFile = UbuntuLocalRunConfiguration::getDesktopFile(this,appId(),errorMessage);
    if(m_desktopFile.isEmpty())
        return false;

    ProjectExplorer::DeployConfiguration *deplConf = qobject_cast<ProjectExplorer::DeployConfiguration*>(target()->activeDeployConfiguration());
    ProjectExplorer::BuildStepList *bsList = deplConf->stepList();
    foreach(ProjectExplorer::BuildStep *currStep ,bsList->steps()) {
        UbuntuPackageStep *pckStep = qobject_cast<UbuntuPackageStep*>(currStep);
        if(!pckStep)
            continue;

        QFileInfo info(pckStep->packagePath());
        if(info.exists()) {
            m_clickPackage = info.fileName();
            break;
        }
    }

    if(m_clickPackage.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("Could not find a click package to run, please check if the deploy configuration has a click package step");
        return false;
    }

    /*
     * Tries to read the Exec line from the desktop file, to
     * extract arguments and to know which "executor" is used on
     * the phone
     */
    QStringList args;
    QString command;

    if(!UbuntuLocalRunConfiguration::readDesktopFile(m_desktopFile,&command,&args,errorMessage))
        return false;

    QFileInfo commInfo(command);
    QString executor = commInfo.completeBaseName();
    if(executor.startsWith(QStringLiteral("qmlscene"))) {
        ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
        if(tc->type() != QString::fromLatin1(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
            if(errorMessage)
                *errorMessage = tr("Wrong toolchain type. Please check your build configuration.");
            return false;
        }

        ClickToolChain* clickTc = static_cast<ClickToolChain*>(tc);
        m_localExecutable  = QString::fromLatin1("%1/usr/lib/%2/qt5/bin/qmlscene")
                .arg(UbuntuClickTool::targetBasePath(clickTc->clickTarget()))
                .arg(clickTc->gnutriplet());

        m_remoteExecutable = QStringLiteral("/usr/bin/qmlscene");
        m_arguments = args;
    } else if(executor.startsWith(QStringLiteral("ubuntu-html5-app-launcher"))
              || executor.startsWith(QStringLiteral("webapp-container"))) {
        m_remoteExecutable = QStringLiteral("");
        m_arguments = args;
    } else {
        //looks like a application without a launcher
        CMakeProjectManager::CMakeProject* pro = static_cast<CMakeProjectManager::CMakeProject*> (target()->project());
        foreach(const CMakeProjectManager::CMakeBuildTarget &t, pro->buildTargets()) {
            if(t.library || t.executable.isEmpty())
                continue;

            QFileInfo execInfo(t.executable);

            if(execInfo.fileName() == commInfo.fileName())
                m_localExecutable = t.executable;
        }

        if (m_localExecutable.isEmpty()) {
            if(errorMessage)
                *errorMessage = tr("Could not find %1 in the project targets").arg(command);
            return false;
        }

        m_remoteExecutable = command;
    }
    return true;
}

bool UbuntuRemoteRunConfiguration::fromMap(const QVariantMap &map)
{
    if(debug) qDebug()<<Q_FUNC_INFO;
    return AbstractRemoteLinuxRunConfiguration::fromMap(map);
}

QVariantMap UbuntuRemoteRunConfiguration::toMap() const
{
    QVariantMap m = AbstractRemoteLinuxRunConfiguration::toMap();
    if(debug) qDebug()<<Q_FUNC_INFO;
    return m;
}

QString UbuntuRemoteRunConfiguration::appId() const
{
    return id().suffixAfter(typeId());
}

QString UbuntuRemoteRunConfiguration::clickPackage() const
{
    return m_clickPackage;
}

Core::Id UbuntuRemoteRunConfiguration::typeId()
{
    return Core::Id("UbuntuProjectManager.RemoteRunConfiguration");
}

void UbuntuRemoteRunConfiguration::setArguments(const QStringList &args)
{
    m_arguments = args;
}

QString UbuntuRemoteRunConfiguration::packageDir() const
{
    ProjectExplorer::Project *p = target()->project();
    if (p->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID)
        return target()->activeBuildConfiguration()->buildDirectory().toString()+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR);
    else if (p->id() == Ubuntu::Constants::UBUNTUPROJECT_ID || p->id() == "QmlProjectManager.QmlProject") {
        if (!target()->activeBuildConfiguration()) {
            //backwards compatibility, try to not crash QtC for old projects
            //they did not create a buildconfiguration back then
            QDir pDir(p->projectDirectory());
            return p->projectDirectory()+QDir::separator()+
                    QStringLiteral("..")+QDir::separator()+
                    pDir.dirName()+QStringLiteral("_build")+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR);
        } else
            return target()->activeBuildConfiguration()->buildDirectory().toString()+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR);
    }
    return QString();
}

} // namespace Internal
} // namespace Ubuntu
