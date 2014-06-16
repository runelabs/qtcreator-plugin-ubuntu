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

#include "ubuntuversionmanager.h"
#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>

using namespace Ubuntu::Internal;

UbuntuVersionManager *UbuntuVersionManager::m_self = 0;

UbuntuVersionManager::UbuntuVersionManager(QObject *parent) :
    QObject(parent)
{
    m_hostVersion = UbuntuVersion::fromLsbFile(QLatin1String(Constants::LSB_RELEASE));
    m_self = this;
}

UbuntuVersionManager::~UbuntuVersionManager()
{
    if(m_hostVersion)
        delete m_hostVersion;
}

void UbuntuVersionManager::detectAvailableVersions() {
    // no other platforms yet, add support for Touch devices.
    // now just the desktop platform.
}

Core::FeatureSet UbuntuVersionManager::availableFeatures(const QString &platformName) const {
    Q_UNUSED(platformName);
    if(m_hostVersion)
        return m_hostVersion->features();

    return Core::FeatureSet();
}

QStringList UbuntuVersionManager::availablePlatforms() const {
    QStringList platforms;
    platforms << QLatin1String(Constants::PLATFORM_DESKTOP);
    return platforms;
}

QString UbuntuVersionManager::displayNameForPlatform(const QString &string) const {
    Q_UNUSED(string);
    return QString(QLatin1String(Constants::PLATFORM_DESKTOP_DISPLAYNAME))
            .arg( (m_hostVersion != 0) ? m_hostVersion->release() : tr("Unknown"));
}

UbuntuVersionManager *UbuntuVersionManager::instance()
{
    return m_self;
}
