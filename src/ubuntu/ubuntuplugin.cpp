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

#include <coreplugin/modemanager.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>

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

    Core::MimeDatabase *mimeDB = Core::ICore::mimeDatabase();

    const QLatin1String mimetypesXml(":/ubuntu/UbuntuProject.mimetypes.xml");

    if (!mimeDB->addMimeTypes(mimetypesXml, errorString))
        return false;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(Internal::UbuntuProjectApplicationWizard::getProjectTypesJSON());
    if (jsonDoc.isArray()) {
        QJsonArray array = jsonDoc.array();
        for (int idx = 0; idx < array.size(); idx++) {
            if (array.at(idx).isObject()) {
                QJsonObject obj = array.at(idx).toObject();
                QString folder;
                QJsonValue tmp_folder = obj.value(QLatin1String(Constants::UBUNTU_PROJECTJSON_FOLDER));
                if (tmp_folder.isUndefined() == false) {
                    folder = tmp_folder.toString();
                    if (QFileInfo(UbuntuProjectApplicationWizard::templatesPath(folder)).exists()) {
                        addAutoReleasedObject(new Internal::UbuntuProjectApplicationWizard(obj));
                    }
                }
            }
        }
    } else {
        qWarning() << __PRETTY_FUNCTION__ << "failed to read from JSON.";
    }

    m_ubuntuDeviceMode = new UbuntuDeviceMode();
    addAutoReleasedObject(m_ubuntuDeviceMode);
    m_ubuntuWelcomeMode = new UbuntuWelcomeMode;
    addAutoReleasedObject(m_ubuntuWelcomeMode);

    m_ubuntuPackagingMode = new UbuntuPackagingMode();
    addAutoReleasedObject(m_ubuntuPackagingMode);

    QSettings settings(QLatin1String("Canonical"),QLatin1String("UbuntuSDK"));
    settings.beginGroup(QLatin1String("Mode"));
    if (settings.value(QLatin1String("API"),true).toBool()) {
        m_ubuntuAPIMode = new UbuntuAPIMode;
        addAutoReleasedObject(m_ubuntuAPIMode);
    }

    if (settings.value(QLatin1String("CoreApps"),true).toBool()) {
        m_ubuntuCoreAppsMode = new UbuntuCoreAppsMode;
        addAutoReleasedObject(m_ubuntuCoreAppsMode);
    }
    if (settings.value(QLatin1String("IRC"),true).toBool()) {
        m_ubuntuIRCMode = new UbuntuIRCMode;
        addAutoReleasedObject(m_ubuntuIRCMode);
    }
    if (settings.value(QLatin1String("Pastebin"),true).toBool()) {
        m_ubuntuPastebinMode = new UbuntuPastebinMode;
        addAutoReleasedObject(m_ubuntuPastebinMode);
    }

    if (settings.value(QLatin1String("Wiki"),true).toBool()) {
        m_ubuntuWikiMode = new UbuntuWikiMode;
        addAutoReleasedObject(m_ubuntuWikiMode);
    }
    settings.endGroup();

    m_ubuntuMenu = new UbuntuMenu;
    addAutoReleasedObject(m_ubuntuMenu);
    addAutoReleasedObject(new UbuntuSettingsPage);

    addAutoReleasedObject(new UbuntuVersionManager);
    addAutoReleasedObject(new UbuntuFeatureProvider);

    //addAutoReleasedObject(new UbuntuProjectManager);
    //addAutoReleasedObject(new UbuntuRunConfigurationFactory);
    //addAutoReleasedObject(new UbuntuRunControlFactory);

    return true;
}

void UbuntuPlugin::extensionsInitialized()
{
    if (m_ubuntuMenu) m_ubuntuMenu->initialize();
    m_ubuntuWelcomeMode->initialize();
    m_ubuntuDeviceMode->initialize();
    if (m_ubuntuIRCMode) m_ubuntuIRCMode->initialize();
    if (m_ubuntuAPIMode) m_ubuntuAPIMode->initialize();
    if (m_ubuntuCoreAppsMode) m_ubuntuCoreAppsMode->initialize();
    if (m_ubuntuWikiMode) m_ubuntuWikiMode->initialize();
    m_ubuntuPackagingMode->initialize();
    Core::ModeManager::activateMode(m_ubuntuWelcomeMode->id());
}

Q_EXPORT_PLUGIN2(Ubuntu, UbuntuPlugin)

