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

    m_ubuntuWelcomeMode = new UbuntuWelcomeMode;
    m_ubuntuDeviceMode = new UbuntuDeviceMode;
    m_ubuntuMenu = new UbuntuMenu;
    m_ubuntuIRCMode = new UbuntuIRCMode;
    m_ubuntuAPIMode = new UbuntuAPIMode;
    m_ubuntuCoreAppsMode = new UbuntuCoreAppsMode;
    m_ubuntuWikiMode = new UbuntuWikiMode;
    m_ubuntuPackagingMode = new UbuntuPackagingMode;
    m_ubuntuPastebinMode = new UbuntuPastebinMode;

    addAutoReleasedObject(m_ubuntuWelcomeMode);
    addAutoReleasedObject(m_ubuntuDeviceMode);
    addAutoReleasedObject(m_ubuntuMenu);
    addAutoReleasedObject(m_ubuntuIRCMode);
    addAutoReleasedObject(m_ubuntuAPIMode);
    addAutoReleasedObject(m_ubuntuCoreAppsMode);
    addAutoReleasedObject(m_ubuntuWikiMode);
    addAutoReleasedObject(m_ubuntuPackagingMode);
    addAutoReleasedObject(m_ubuntuPastebinMode);

    addAutoReleasedObject(new UbuntuVersionManager);
    addAutoReleasedObject(new UbuntuFeatureProvider);

    //addAutoReleasedObject(new UbuntuProjectManager);
    //addAutoReleasedObject(new UbuntuRunConfigurationFactory);
    //addAutoReleasedObject(new UbuntuRunControlFactory);

    return true;
}

void UbuntuPlugin::extensionsInitialized()
{
    m_ubuntuMenu->initialize();
    m_ubuntuWelcomeMode->initialize();
    m_ubuntuDeviceMode->initialize();
    m_ubuntuIRCMode->initialize();
    m_ubuntuAPIMode->initialize();
    m_ubuntuCoreAppsMode->initialize();
    m_ubuntuWikiMode->initialize();
    m_ubuntuPackagingMode->initialize();
    Core::ModeManager::activateMode(m_ubuntuWelcomeMode->id());
}

Q_EXPORT_PLUGIN2(Ubuntu, UbuntuPlugin)

