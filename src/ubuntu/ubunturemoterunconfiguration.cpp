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

    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if(tc->type() == QString::fromLatin1(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
        ClickToolChain* clickTc = static_cast<ClickToolChain *>(tc);
        env.appendOrSet(QLatin1String("gnutriplet"),clickTc->gnutriplet());
        env.appendOrSet(QLatin1String("QML2_IMPORT_PATH"),
                        QString::fromLatin1("lib/%1").arg(clickTc->gnutriplet()),
                        QString::fromLatin1(":"));
        env.prependOrSetPath(QStringLiteral("lib/")+clickTc->gnutriplet()+QStringLiteral("/bin"));
        env.appendOrSetPath(QStringLiteral("$PATH"));
    }
    env.appendOrSet(QLatin1String("pkgdir"),workingDirectory());
    env.appendOrSet(QLatin1String("APP_ID"),m_appId);

    QFileInfo desk(m_desktopFile);
    env.set(QStringLiteral("QTC_DESKTOP_PATH"),
            QStringLiteral("/home/phablet/dev_tmp/%1/%2")
            .arg(target()->project()->displayName())
            .arg(desk.fileName()));

    env.set(QStringLiteral("QTC_DESKTOP_DEST"),
            QStringLiteral("/home/phablet/.local/share/applications/%1")
            .arg(desk.fileName()));

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
    m_arguments.clear();
    m_desktopFile.clear();
    m_appId.clear();

    QDir package_dir(target()->activeBuildConfiguration()->buildDirectory().toString()+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR));
    if(!package_dir.exists()) {
        if(errorMessage)
            *errorMessage = tr("No packaging directory available, please check if the deploy configuration is correct.");

        return false;
    }

    m_desktopFile = UbuntuLocalRunConfiguration::getDesktopFile(this,&m_appId,errorMessage);
    if(m_desktopFile.isEmpty())
        return false;

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
    if(commInfo.completeBaseName().startsWith(QLatin1String("qmlscene"))) {
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

        m_remoteExecutable = QLatin1String("/usr/bin/qmlscene");
        m_arguments = args;
    } else if(commInfo.completeBaseName().startsWith(QLatin1String("ubuntu-html5-app-launcher"))) {
        if(errorMessage)
            *errorMessage = tr("Run support for remote html projects not available");
        return false;
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

    QFileInfo desk(m_desktopFile);
    m_arguments.append(QString::fromLatin1("--desktop_file_hint=/home/phablet/.local/share/applications/%2")
                            .arg(target()->project()->displayName())
                            .arg(desk.fileName()));

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

void UbuntuRemoteRunConfiguration::setArguments(const QStringList &args)
{
    m_arguments = args;
}

} // namespace Internal
} // namespace Ubuntu
