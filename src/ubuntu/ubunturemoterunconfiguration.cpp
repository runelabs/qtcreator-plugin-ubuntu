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
#include "ubuntuconstants.h"
#include "ubuntulocalrunconfiguration.h"
#include "ubunturemotedeployconfiguration.h"
#include "ubuntupackagestep.h"
#include "ubuntuclickmanifest.h"
#include "ui_ubunturemoterunconfigurationwidget.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildsteplist.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>
#include <utils/qtcprocess.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QVariantMap>
#include <QVariant>
#include <QFileInfo>
#include <QSettings>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

const char FORCE_INSTALL_KEY[]="UbuntuRemoteRunConfiguration.ForceInstall";
const char UNINSTALL_KEY[]="UbuntuRemoteRunConfiguration.Uninstall";

UbuntuRemoteRunConfiguration::UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent, Core::Id id)
    : AbstractRemoteLinuxRunConfiguration(parent,id),
      m_running(false),
      m_forceInstall(false),
      m_uninstall(true)
{
    setDisplayName(appId());
    addExtraAspect(new RemoteLinux::RemoteLinuxEnvironmentAspect(this));
}

UbuntuRemoteRunConfiguration::UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent, UbuntuRemoteRunConfiguration *source)
    : AbstractRemoteLinuxRunConfiguration(parent,source),
      m_running(false),
      m_forceInstall(false),
      m_uninstall(true)
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

    //work around a problem with loading OPENSSL triggering a SIGILL, injecting that
    //environment variable disables the check that causes the problem
    if(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit())
            == Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix(QStringLiteral("armhf"))) {
        env.set(QStringLiteral("OPENSSL_armcap"),QStringLiteral("0"));
    }

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
    } else {
        QmakeProjectManager::QmakeProject* qmakeProj =
                qobject_cast<QmakeProjectManager::QmakeProject *>(target()->project());
        if(qmakeProj) {
            foreach (const QmakeProjectManager::QmakeProFileNode* pro, qmakeProj->allProFiles()) {
                if(pro->projectType() == QmakeProjectManager::ApplicationTemplate
                         || pro->projectType() == QmakeProjectManager::LibraryTemplate) {
                    QmakeProjectManager::TargetInformation info = pro->targetInformation();
                    if(!info.valid)
                        continue;
                    if(debug) qDebug()<<"Adding path "<<info.buildDir<<" to solib-search-paths";
                    paths << info.buildDir;
                }
            }
        }
    }

    return paths;
}

QWidget *UbuntuRemoteRunConfiguration::createConfigurationWidget()
{
    return new UbuntuRemoteRunConfigurationWidget(this);
}

bool UbuntuRemoteRunConfiguration::isEnabled() const
{
    return (!m_running);
}

QString UbuntuRemoteRunConfiguration::disabledReason() const
{
    if(m_running)
        return tr("This configuration is already running on the device");
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
ProjectExplorer::RunConfiguration::ConfigurationState UbuntuRemoteRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if(debug) qDebug()<<"--------------------- Reconfiguring RunConfiguration ----------------------------";
    m_arguments.clear();
    m_clickPackage.clear();

    QDir package_dir(packageDir());
    if(!package_dir.exists()) {
        if(errorMessage)
            *errorMessage = tr("No packaging directory available, please check if the deploy configuration is correct.");

        return ProjectExplorer::RunConfiguration::UnConfigured;
    }

    ProjectExplorer::DeployConfiguration *deplConf = qobject_cast<ProjectExplorer::DeployConfiguration*>(target()->activeDeployConfiguration());
    if(!deplConf) {
        if(errorMessage)
            *errorMessage = tr("No valid deploy configuration is set.");

        return ProjectExplorer::RunConfiguration::UnConfigured;
    }

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
        return ProjectExplorer::RunConfiguration::UnConfigured;
    }

    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if(tc->type() != QString::fromLatin1(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
        if(errorMessage)
            *errorMessage = tr("Wrong toolchain type. Please check your build configuration.");
        return ProjectExplorer::RunConfiguration::UnConfigured;
    }

    ClickToolChain* clickTc = static_cast<ClickToolChain*>(tc);

    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();
    if(!bc) {
        if(errorMessage)
            *errorMessage = tr("Invalid buildconfiguration");
        return ProjectExplorer::RunConfiguration::UnConfigured;
    }

    if(id().toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_APP_ID))) {

        QString desktopFile = UbuntuLocalRunConfiguration::getDesktopFile(this,appId(),errorMessage);
        if(desktopFile.isEmpty())
            return ProjectExplorer::RunConfiguration::UnConfigured;
        /*
         * Tries to read the Exec line from the desktop file, to
         * extract arguments and to know which "executor" is used on
         * the phone
         */
        QStringList args;
        QString command;

        if(!UbuntuLocalRunConfiguration::readDesktopFile(desktopFile,&command,&args,errorMessage))
            return ProjectExplorer::RunConfiguration::UnConfigured;

        QFileInfo commInfo(command);
        QString executor = commInfo.completeBaseName();
        if(executor.startsWith(QStringLiteral("qmlscene"))) {
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
            if(target()->project()->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID) {
                QmakeProjectManager::QmakeProject* pro = static_cast<QmakeProjectManager::QmakeProject*> (target()->project());
                foreach(const QmakeProjectManager::QmakeProFileNode* applPro, pro->applicationProFiles()) {
                    QmakeProjectManager::TargetInformation info = applPro->targetInformation();
                    if(applPro->targetInformation().valid) {
                        if(info.target == commInfo.fileName()) {
                            m_localExecutable = info.buildDir + QDir::separator() + info.target;
                            break;
                        }
                    }
                }
            } else {
                CMakeProjectManager::CMakeProject* pro = static_cast<CMakeProjectManager::CMakeProject*> (target()->project());
                foreach(const CMakeProjectManager::CMakeBuildTarget &t, pro->buildTargets()) {
                    if(!t.targetType != CMakeProjectManager::ExecutableType|| t.executable.isEmpty())
                        continue;

                    QFileInfo execInfo(t.executable);

                    if(execInfo.fileName() == commInfo.fileName())
                        m_localExecutable = t.executable;
                }
            }

            if (m_localExecutable.isEmpty()) {
                if(errorMessage)
                    *errorMessage = tr("Could not find %1 in the project targets").arg(command);
                return ProjectExplorer::RunConfiguration::UnConfigured;
            }
            m_remoteExecutable = command;
        }
        return ProjectExplorer::RunConfiguration::Configured;
    } else if (id().toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_SCOPE_ID))) {

        QDir package_dir(packageDir());
        if(!package_dir.exists()) {
            if(errorMessage)
                *errorMessage = tr("No packaging directory available, please check if the deploy configuration is correct.");
            return ProjectExplorer::RunConfiguration::UnConfigured;
        }

        QString manifestPath = package_dir.absoluteFilePath(QStringLiteral("manifest.json"));

        //read the manifest
        if(!QFile::exists(manifestPath)) {
            if(errorMessage)
                *errorMessage = tr("Could not find the manifest file in the package directory, make sure its installed into the root of the click package.");
            return ProjectExplorer::RunConfiguration::UnConfigured;
        }

        UbuntuClickManifest manifest;
        QString manifestErrMsg;
        if(!manifest.load(manifestPath,nullptr,&manifestErrMsg)) {
            if(errorMessage)
                *errorMessage = tr("Could not read the manifest file in the package directory: %1").arg(manifestErrMsg);
            return ProjectExplorer::RunConfiguration::UnConfigured;
        }

        QString iniFilePath;
        for(const UbuntuClickManifest::Hook &hook : manifest.hooks()) {
            if(hook.appId == appId()) {
                iniFilePath = bc->buildDirectory()
                        .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                        .appendPath(hook.scope)
                        .appendPath(manifest.name()+QStringLiteral("_")+hook.appId+QStringLiteral(".ini"))
                        .toString();
            }
        }

        if(iniFilePath.isEmpty()) {
            if(errorMessage)
                *errorMessage = tr("Could not find a hook with id %1 in the manifest file.").arg(appId());
            return ProjectExplorer::RunConfiguration::UnConfigured;
        }

        if(!QFile::exists(iniFilePath)){
            if(errorMessage)
                *errorMessage = tr("Ini file for scope: %1 does not exist.").arg(appId());
            return ProjectExplorer::RunConfiguration::UnConfigured;
        }

        QSettings iniFile(iniFilePath,QSettings::IniFormat);
        if(iniFile.status() != QSettings::NoError) {
            if(errorMessage)
                *errorMessage = tr("Could not read the ini file for scope: .").arg(appId());
            return ProjectExplorer::RunConfiguration::UnConfigured;
        }

        iniFile.beginGroup(QStringLiteral("ScopeConfig"));

        //the default exec line
        QString execLine = QStringLiteral("/usr/lib/%1/unity-scopes/scoperunner %R %S").arg(static_cast<ClickToolChain*>(tc)->gnutriplet());

        QString srKey(QStringLiteral("ScopeRunner"));
        if(iniFile.contains(srKey))
            execLine = iniFile.value(srKey).toString();

        QStringList args = Utils::QtcProcess::splitArgs(execLine);
        QString executable = args.takeFirst();

        //if debugging is enabled we inject the debughelper, so we need
        //to remove it here
        if(executable.contains(QStringLiteral("qtc_device_debughelper.py"))) {
            //remove the mode and the scope id argument
            args.takeFirst();
            args.takeFirst();
            args = Utils::QtcProcess::splitArgs(args[0]);
            executable = args.takeFirst();
        }

        QFileInfo commandInfo(executable);
        if(commandInfo.completeBaseName().startsWith(QStringLiteral("scoperunner"))) {
            m_localExecutable  = QString::fromLatin1("%1/usr/lib/%2/unity-scopes/scoperunner")
                    .arg(UbuntuClickTool::targetBasePath(clickTc->clickTarget()))
                    .arg(clickTc->gnutriplet());

            m_remoteExecutable = QStringLiteral("/usr/lib/%1/unity-scopes/scoperunner").arg(clickTc->gnutriplet());
            m_arguments = args;
            return ProjectExplorer::RunConfiguration::Configured;
        } else {
            if(errorMessage)
                *errorMessage = tr("Using a custom scopelauncher is not yet supported");
            return ProjectExplorer::RunConfiguration::UnConfigured;
        }
    }

    if(errorMessage)
        *errorMessage = tr("Incompatible runconfiguration type id");
    return ProjectExplorer::RunConfiguration::UnConfigured;
}

bool UbuntuRemoteRunConfiguration::fromMap(const QVariantMap &map)
{
    if(debug) qDebug()<<Q_FUNC_INFO;

    if(!AbstractRemoteLinuxRunConfiguration::fromMap(map))
        return false;

    m_uninstall = map.value(QLatin1String(UNINSTALL_KEY),true).toBool();
    m_forceInstall = map.value(QLatin1String(FORCE_INSTALL_KEY),false).toBool();
    return true;
}

QVariantMap UbuntuRemoteRunConfiguration::toMap() const
{
    if(debug) qDebug()<<Q_FUNC_INFO;
    QVariantMap m = AbstractRemoteLinuxRunConfiguration::toMap();
    m.insert(QLatin1String(UNINSTALL_KEY),m_uninstall);
    m.insert(QLatin1String(FORCE_INSTALL_KEY),m_forceInstall);
    return m;
}

QString UbuntuRemoteRunConfiguration::appId() const
{
    if(id().toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_APP_ID)))
        return id().suffixAfter(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_APP_ID);
    else
        return id().suffixAfter(Constants::UBUNTUPROJECT_REMOTE_RUNCONTROL_SCOPE_ID);
}

QString UbuntuRemoteRunConfiguration::clickPackage() const
{
    return m_clickPackage;
}

void UbuntuRemoteRunConfiguration::setArguments(const QStringList &args)
{
    m_arguments = args;
}

QString UbuntuRemoteRunConfiguration::packageDir() const
{
    ProjectExplorer::Project *p = target()->project();
    if (p->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID || p->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID)
        return target()->activeBuildConfiguration()->buildDirectory().toString()+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR);
    else if (p->id() == Ubuntu::Constants::UBUNTUPROJECT_ID || p->id() == "QmlProjectManager.QmlProject") {
        if (!target()->activeBuildConfiguration()) {
            //backwards compatibility, try to not crash QtC for old projects
            //they did not create a buildconfiguration back then
            QDir pDir(p->projectDirectory().toString());
            return p->projectDirectory()
                    .appendPath(QStringLiteral(".."))
                    .appendPath(pDir.dirName()+QStringLiteral("_build"))
                    .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR)).toString();
        } else
            return target()->activeBuildConfiguration()->buildDirectory()
                    .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR)).toString();
    }
    return QString();
}

void UbuntuRemoteRunConfiguration::setRunning(const bool set)
{
    if(m_running != set) {
        m_running = set;
        emit enabledChanged();
    }
}
bool UbuntuRemoteRunConfiguration::forceInstall() const
{
    return m_forceInstall;
}

void UbuntuRemoteRunConfiguration::setForceInstall(bool forceInstall)
{
    if(m_forceInstall == forceInstall)
        return;

    m_forceInstall = forceInstall;
    emit forceInstallChanged(forceInstall);
}
bool UbuntuRemoteRunConfiguration::uninstall() const
{
    return m_uninstall;
}

void UbuntuRemoteRunConfiguration::setUninstall(bool uninstall)
{
    if(m_uninstall == uninstall)
        return;

    m_uninstall = uninstall;
    emit uninstallChanged(uninstall);
}



UbuntuRemoteRunConfigurationWidget::UbuntuRemoteRunConfigurationWidget(UbuntuRemoteRunConfiguration *config, QWidget *parent)
    : QWidget(parent),
      m_config(config)
{
    m_ui = new Ui::UbuntuRemoteRunconfigurationWidget;
    m_ui->setupUi(this);

    m_ui->checkBoxForceInstall->setChecked(config->forceInstall());
    m_ui->checkBoxUninstall->setChecked(config->uninstall());
    connect(m_ui->checkBoxForceInstall,&QCheckBox::toggled,config,&UbuntuRemoteRunConfiguration::setForceInstall);
    connect(m_ui->checkBoxUninstall,&QCheckBox::toggled,config,&UbuntuRemoteRunConfiguration::setUninstall);
    connect(config,&UbuntuRemoteRunConfiguration::forceInstallChanged,m_ui->checkBoxForceInstall,&QCheckBox::setChecked);
    connect(config,&UbuntuRemoteRunConfiguration::uninstallChanged,m_ui->checkBoxUninstall,&QCheckBox::setChecked);
}

UbuntuRemoteRunConfigurationWidget::~UbuntuRemoteRunConfigurationWidget()
{
    delete m_ui;
}

} // namespace Internal
} // namespace Ubuntu
