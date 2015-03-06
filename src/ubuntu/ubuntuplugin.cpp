/*
 * Copyright 2013 Canonical Ltd.
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
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#include "ubuntuplugin.h"
#include "ubuntuconstants.h"
#include "ubuntuprojectmanager.h"
#include "ubuntulocalrunconfiguration.h"
#include "ubuntulocalrunconfigurationfactory.h"
#include "ubunturemoteruncontrolfactory.h"
#include "ubuntuclicktool.h"
#include "ubuntukitmanager.h"
#include "ubuntudevicefactory.h"
#include "clicktoolchain.h"
#include "ubuntuhtmlbuildconfiguration.h"
#include "ubunturemotedeployconfiguration.h"
#include "ubuntulocaldeployconfiguration.h"
#include "ubuntudevicesmodel.h"
#include "localportsmanager.h"
#include "ubuntubzr.h"
#include "ubuntuqtversion.h"
#include "ubuntudeploystepfactory.h"
#include "ubuntuqmlbuildconfiguration.h"
#include "ubuntueditorfactory.h"
#include "ubuntucmakecache.h"
#include "ubuntutestcontrol.h"
#include "ubuntupackageoutputparser.h"
#include "ubuntuprojecthelper.h"
#include "ubuntuscopefinalizer.h"
#include "targetupgrademanager.h"

#include "wizards/ubuntuprojectapplicationwizard.h"
#include "wizards/ubuntufirstrunwizard.h"
#include "wizards/ubuntuprojectmigrationwizard.h"

#include <coreplugin/modemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projecttree.h>
#include <coreplugin/featureprovider.h>
#include <coreplugin/coreplugin.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimeglobpattern_p.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QtQml>
#include <QFile>
#include <QAction>
#include <QProcess>

#include <coreplugin/icore.h>
#include <stdint.h>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

UbuntuPlugin::UbuntuPlugin()
{
    if(UbuntuClickTool::clickChrootSuffix() == QLatin1String(Constants::UBUNTU_CLICK_CHROOT_DEFAULT_NAME)) {
#ifdef UBUNTU_BUILD_ROOT
        Utils::FileName chrootAgent = Utils::FileName::fromString(QStringLiteral(UBUNTU_BUILD_ROOT));
        chrootAgent.appendPath(QStringLiteral("chroot-agent"))  //append dir
                .appendPath(QStringLiteral("click-chroot-agent")); //append binary

        bool started = false;
        if(chrootAgent.toFileInfo().isExecutable()) {
            started = QProcess::startDetached(chrootAgent.toFileInfo().absoluteFilePath());
        }
        if(!started) {
            QProcess::startDetached(QStringLiteral("click-chroot-agent"));
        }
#else
        //start the chroot-agent
        QProcess::startDetached(QStringLiteral("click-chroot-agent"));
#endif
    }
}

UbuntuPlugin::~UbuntuPlugin()
{
}

bool UbuntuPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    qmlRegisterUncreatableType<UbuntuQmlDeviceConnectionState>("Ubuntu.DevicesModel",0,1,"DeviceConnectionState",QStringLiteral("Not instantiable"));
    qmlRegisterUncreatableType<UbuntuQmlDeviceDetectionState>("Ubuntu.DevicesModel",0,1,"DeviceDetectionState",QStringLiteral("Not instantiable"));
    qmlRegisterUncreatableType<UbuntuQmlFeatureState>("Ubuntu.DevicesModel",0,1,"FeatureState",QStringLiteral("Not instantiable"));
    qmlRegisterUncreatableType<UbuntuQmlDeviceMachineType>("Ubuntu.DevicesModel",0,1,"DeviceMachineType",QStringLiteral("Not instantiable"));

    //create .config/ubuntu-sdk directory if it does not exist
    QString confdir = QStringLiteral("%1/.config/ubuntu-sdk")
            .arg(QDir::homePath());

    QDir d = QDir::root();
    if(!d.exists(confdir)) {
        if(!d.mkpath(confdir))
            qWarning()<<"Unable to create Ubuntu-SDK configuration directory "<<confdir;
    }

    Utils::MimeDatabase::addMimeTypes(QLatin1String(Constants::UBUNTU_MIMETYPE_XML));

    addAutoReleasedObject(new UbuntuClickFrameworkProvider);

    m_ubuntuDeviceMode = new UbuntuDeviceMode();
    addAutoReleasedObject(m_ubuntuDeviceMode);

    addAutoReleasedObject(new UbuntuBzr);

    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_MODE));

    if (settings.value(QLatin1String(Constants::SETTINGS_KEY_API),Constants::SETTINGS_DEFAULT_API_VISIBILITY).toBool()) {
        m_ubuntuAPIMode = new UbuntuAPIMode;
        addAutoReleasedObject(m_ubuntuAPIMode);
    }

    if (settings.value(QLatin1String(Constants::SETTINGS_KEY_COREAPPS),Constants::SETTINGS_DEFAULT_COREAPPS_VISIBILITY).toBool()) {
        m_ubuntuCoreAppsMode = new UbuntuCoreAppsMode;
        addAutoReleasedObject(m_ubuntuCoreAppsMode);
    }
    if (settings.value(QLatin1String(Constants::SETTINGS_KEY_IRC),Constants::SETTINGS_DEFAULT_IRC_VISIBILITY).toBool()) {
        m_ubuntuIRCMode = new UbuntuIRCMode;
        addAutoReleasedObject(m_ubuntuIRCMode);
    }
    if (settings.value(QLatin1String(Constants::SETTINGS_KEY_PASTEBIN),Constants::SETTINGS_DEFAULT_PASTEBIN_VISIBILITY).toBool()) {
        m_ubuntuPastebinMode = new UbuntuPastebinMode;
        addAutoReleasedObject(m_ubuntuPastebinMode);
    }

    if (settings.value(QLatin1String(Constants::SETTINGS_KEY_WIKI),Constants::SETTINGS_DEFAULT_WIKI_VISIBILITY).toBool()) {
        m_ubuntuWikiMode = new UbuntuWikiMode;
        addAutoReleasedObject(m_ubuntuWikiMode);
    }

    settings.endGroup();

    m_ubuntuMenu = new UbuntuMenu;
    addAutoReleasedObject(m_ubuntuMenu);

    m_ubuntuPackagingMode = new UbuntuPackagingMode();
    addAutoReleasedObject(m_ubuntuPackagingMode);

    addAutoReleasedObject(new UbuntuSettingsClickPage);
    addAutoReleasedObject(new UbuntuSettingsDeviceConnectivityPage);
    addAutoReleasedObject(new UbuntuSettingsPage);

    addAutoReleasedObject(new UbuntuVersionManager);
    Core::IWizardFactory::registerFeatureProvider(new UbuntuFeatureProvider);

    // welcome page plugin
    addAutoReleasedObject(new UbuntuWelcomePage);

    // Handle new project type files
    addAutoReleasedObject(new UbuntuProjectManager);
    addAutoReleasedObject(new UbuntuLocalRunConfigurationFactory);
    addAutoReleasedObject(new UbuntuRemoteRunControlFactory);

    // Build support
    addAutoReleasedObject(new ClickToolChainFactory);
    addAutoReleasedObject(new UbuntuCMakeCache);
    addAutoReleasedObject(new UbuntuHtmlBuildConfigurationFactory);
    addAutoReleasedObject(new UbuntuQmlBuildConfigurationFactory);
    addAutoReleasedObject(new UbuntuQmlBuildStepFactory);

    CMakeProjectManager::CMakeToolManager::registerAutodetectionHelper([](){
        QList<CMakeProjectManager::CMakeTool *> found;

        QList<ClickToolChain *> uTcs = UbuntuKitManager::clickToolChains();
        foreach(ClickToolChain *tc, uTcs) {
            CMakeProjectManager::CMakeTool *tool = UbuntuKitManager::createCMakeTool(tc);
            if (tool)
                found.append(tool);
        }
        return found;
    });

    //ubuntu device support
    addAutoReleasedObject(new UbuntuDeviceFactory);
    addAutoReleasedObject(new UbuntuLocalPortsManager);

    //deploy support
    addAutoReleasedObject(new UbuntuRemoteDeployConfigurationFactory);
    addAutoReleasedObject(new UbuntuClickReviewTaskHandler);

    //register wizards
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::CMakeProject>(
                    QStringLiteral("ubuntu-project-cmake"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::QMakeProject>(
                    QStringLiteral("ubuntu-project-qmake"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::UbuntuHTMLProject>(
                    QStringLiteral("ubuntu-project-plain-html"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::UbuntuQMLProject>(
                    QStringLiteral("ubuntu-project-plain-qml"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::GoProject>(
                    QStringLiteral("ubuntu-project-go"),
                    Core::IWizardFactory::ProjectWizard));

    //register Qt version
    addAutoReleasedObject(new Ubuntu::Internal::UbuntuQtVersionFactory);

    //disabled for now, keeping the code because we might need a deploy method
    //for local applications in the future
    //addAutoReleasedObject(new UbuntuLocalDeployConfigurationFactory);
    addAutoReleasedObject(new UbuntuDeployStepFactory);

    addAutoReleasedObject(new Internal::UbuntuManifestEditorFactory);
    addAutoReleasedObject(new Internal::UbuntuApparmorEditorFactory);

    //trigger kit autodetection and update after projectexplorer loaded the kits
    connect(ProjectExplorer::KitManager::instance(),SIGNAL(kitsLoaded())
            ,this,SLOT(onKitsLoaded()));


    const Core::Context qmakeProjectContext(QmakeProjectManager::Constants::PROJECT_ID);

    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    //support for the UbuntuProjectMigrateWizard
    connect(ProjectExplorer::ProjectTree::instance(), SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)),
            this, SLOT(updateContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)));

    m_migrateProjectAction = new QAction(tr("Migrate to Ubuntu project"), this);
    Core::Command *command = Core::ActionManager::registerAction(m_migrateProjectAction, Constants::UBUNTU_MIGRATE_QMAKE_PROJECT, qmakeProjectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    connect(m_migrateProjectAction, SIGNAL(triggered()), this, SLOT(migrateProject()));

    return true;
}

void UbuntuPlugin::extensionsInitialized()
{
    if (m_ubuntuMenu) m_ubuntuMenu->initialize();
    m_ubuntuDeviceMode->initialize();
    if (m_ubuntuIRCMode) m_ubuntuIRCMode->initialize();
    if (m_ubuntuAPIMode) m_ubuntuAPIMode->initialize();
    if (m_ubuntuCoreAppsMode) m_ubuntuCoreAppsMode->initialize();
    if (m_ubuntuWikiMode) m_ubuntuWikiMode->initialize();
    m_ubuntuPackagingMode->initialize();

    //add the create click package menu item to the project context menu
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    if(mproject) {
        Core::Command *comm = Core::ActionManager::command("Ubuntu.Build.CreateClickPackage");
        if(comm)
            mproject->addAction(comm, ProjectExplorer::Constants::G_PROJECT_BUILD);

        comm = Core::ActionManager::command("Ubuntu.Build.CreateManifest");
        if(comm)
            mproject->addAction(comm, ProjectExplorer::Constants::G_PROJECT_BUILD);
    }

    //add ubuntu testcontrol to the object tree
    new UbuntuTestControl(Core::ICore::mainWindow());
}

void UbuntuPlugin::onKitsLoaded()
{
    UbuntuKitManager::autoDetectKits();
    disconnect(ProjectExplorer::KitManager::instance(),SIGNAL(kitsLoaded())
               ,this,SLOT(onKitsLoaded()));

    showFirstStartWizard();

    TargetUpgradeManager *mgr = new TargetUpgradeManager();
    addAutoReleasedObject(mgr);
    mgr->checkForUpgrades();
}

void UbuntuPlugin::showFirstStartWizard()
{
    QString file = QStringLiteral("%1/.config/ubuntu-sdk/firstrun")
            .arg(QDir::homePath());

    if(!QFile::exists(file)) {
        UbuntuFirstRunWizard wiz(Core::ICore::mainWindow());
        if( wiz.exec() == QDialog::Accepted ) {
            if (wiz.field(QStringLiteral("createEmulator")).toBool()) {
                Core::ModeManager::activateMode(Ubuntu::Constants::UBUNTU_MODE_DEVICES);

                //invoke the method the next time the event loop starts
                QMetaObject::invokeMethod(m_ubuntuDeviceMode,"showAddEmulatorDialog",Qt::QueuedConnection);
            }
        }

        if(wiz.field(QStringLiteral("disableWizard")).toBool()) {
            QFile f(file);
            if(f.open(QIODevice::WriteOnly)) {
                f.write("1");
                f.close();
            }
        }
    }
}

void UbuntuPlugin::updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node)
{
    m_currentContextMenuProject = project;
    m_migrateProjectAction->setVisible(false);

    QmakeProjectManager::QmakeProject *qProject = qobject_cast<QmakeProjectManager::QmakeProject *>(project);
    QmakeProjectManager::QmakeProFileNode *qNode = static_cast<QmakeProjectManager::QmakeProFileNode *>(node);
    if(qProject && qNode) {
        if(qProject->rootProjectNode() == qNode &&
                UbuntuProjectHelper::getManifestPath(project,QString()).isEmpty()) {
            auto projectType = qNode->projectType();
            if(projectType == QmakeProjectManager::ApplicationTemplate
                    || projectType == QmakeProjectManager::SubDirsTemplate) {
                m_migrateProjectAction->setVisible(true);
            }
        }
    }
}

void UbuntuPlugin::migrateProject()
{
    QmakeProjectManager::QmakeProject *p = qobject_cast<QmakeProjectManager::QmakeProject *>(m_currentContextMenuProject);
    if(!p)
        return;

    UbuntuProjectMigrationWizard::doMigrateProject(p,Core::ICore::mainWindow());
}
