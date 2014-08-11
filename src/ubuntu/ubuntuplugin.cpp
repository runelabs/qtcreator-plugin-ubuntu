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
#include "ubuntuprojectapplicationwizard.h"
#include "ubuntuprojectmanager.h"
#include "ubuntulocalrunconfiguration.h"
#include "ubuntulocalrunconfigurationfactory.h"
#include "ubunturemoteruncontrolfactory.h"
#include "ubuntuclicktool.h"
#include "ubuntukitmanager.h"
#include "ubuntucmaketool.h"
#include "ubuntudevicefactory.h"
#include "clicktoolchain.h"
#include "ubuntucmakebuildconfiguration.h"
#include "ubuntuhtmlbuildconfiguration.h"
#include "ubunturemotedeployconfiguration.h"
#include "ubuntulocaldeployconfiguration.h"
#include "ubuntudevicesmodel.h"
#include "localportsmanager.h"
#include "ubuntubzr.h"
#include "ubuntuqtversion.h"
#include "ubuntudeploystepfactory.h"
#include "ubuntuqmlbuildconfiguration.h"
#include "ubuntufirstrunwizard.h"

#include <coreplugin/modemanager.h>
#include <projectexplorer/kitmanager.h>
#include <coreplugin/featureprovider.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QtQml>
#include <QFile>
#include <coreplugin/icore.h>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

UbuntuPlugin::UbuntuPlugin()
{

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

    const QLatin1String mimetypesXml(Constants::UBUNTU_MIMETYPE_XML);
    if (!Core::MimeDatabase::addMimeTypes(mimetypesXml, errorString))
        return false;

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
    addAutoReleasedObject(new UbuntuFeatureProvider);

    // welcome page plugin
    addAutoReleasedObject(new UbuntuWelcomePage);

    // Handle new project type files
    addAutoReleasedObject(new UbuntuProjectManager);
    addAutoReleasedObject(new UbuntuLocalRunConfigurationFactory);
    addAutoReleasedObject(new UbuntuRemoteRunControlFactory);

    // Build support
    addAutoReleasedObject(new ClickToolChainFactory);
    addAutoReleasedObject(new UbuntuCMakeToolFactory);
    addAutoReleasedObject(new UbuntuCMakeMakeStepFactory);
    addAutoReleasedObject(new UbuntuCMakeBuildConfigurationFactory);
    addAutoReleasedObject(new UbuntuHtmlBuildConfigurationFactory);
    addAutoReleasedObject(new UbuntuQmlBuildConfigurationFactory);

    //ubuntu device support
    addAutoReleasedObject(new UbuntuDeviceFactory);
    addAutoReleasedObject(new UbuntuLocalPortsManager);

    //deploy support
    addAutoReleasedObject(new UbuntuRemoteDeployConfigurationFactory);

    //register wizards
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::CMakeProject>(
                    QStringLiteral("ubuntu-project-cmake"),
                    Core::IWizard::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::UbuntuHTMLProject>(
                    QStringLiteral("ubuntu-project-plain-html"),
                    Core::IWizard::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::UbuntuQMLProject>(
                    QStringLiteral("ubuntu-project-plain-qml"),
                    Core::IWizard::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::GoProject>(
                    QStringLiteral("ubuntu-project-go"),
                    Core::IWizard::ProjectWizard));

    //register Qt version
    addAutoReleasedObject(new Ubuntu::Internal::UbuntuQtVersionFactory);

    //disabled for now, keeping the code because we might need a deploy method
    //for local applications in the future
    //addAutoReleasedObject(new UbuntuLocalDeployConfigurationFactory);
    addAutoReleasedObject(new UbuntuDeployStepFactory);

    //trigger kit autodetection and update after projectexplorer loaded the kits
    connect(ProjectExplorer::KitManager::instance(),SIGNAL(kitsLoaded())
            ,this,SLOT(onKitsLoaded()));

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
}

void UbuntuPlugin::onKitsLoaded()
{
    UbuntuKitManager::autoDetectKits();
    disconnect(ProjectExplorer::KitManager::instance(),SIGNAL(kitsLoaded())
               ,this,SLOT(onKitsLoaded()));

    showFirstStartWizard();
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

Q_EXPORT_PLUGIN2(Ubuntu, UbuntuPlugin)

