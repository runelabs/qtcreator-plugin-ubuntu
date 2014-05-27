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

#include "ubuntuversion.h"

#include <QFile>
#include <QStringList>

using namespace Ubuntu::Internal;

UbuntuVersion::UbuntuVersion()
{

}

Core::FeatureSet UbuntuVersion::features() {
    Core::FeatureSet retval;

    QString cName = codename();
    if (cName==QLatin1String(Constants::PRECISE)) {
        retval |= Core::FeatureSet(Constants::FEATURE_UBUNTU_PRECISE);
    } else if (cName==QLatin1String(Constants::QUANTAL)) {
        retval |= Core::FeatureSet(Constants::FEATURE_UBUNTU_QUANTAL);
    } else if (cName==QLatin1String(Constants::RARING)) {
        retval |= Core::FeatureSet(Constants::FEATURE_UBUNTU_RARING);
    } else if (cName==QLatin1String(Constants::SAUCY)) {
        retval |= Core::FeatureSet(Constants::FEATURE_UBUNTU_SAUCY);
        retval |= Core::FeatureSet(Constants::FEATURE_UNITY_SCOPE);
    } else if  (cName==QLatin1String(Constants::TRUSTY)) {
        retval |= Core::FeatureSet(Constants::FEATURE_UBUNTU_TRUSTY);
        retval |= Core::FeatureSet(Constants::FEATURE_UNITY_SCOPE);
    } else if (cName==QLatin1String(Constants::UTOPIC)) {
        retval |= Core::FeatureSet(Constants::FEATURE_UBUNTU_UTOPIC);
        retval |= Core::FeatureSet(Constants::FEATURE_UNITY_SCOPE);
    }
    return retval;
}

UbuntuVersion *UbuntuVersion::fromLsbFile(const QString &fileName)
{
    QFile lsbRelease(fileName);
    if (lsbRelease.open(QIODevice::ReadOnly)) {
        QByteArray data = lsbRelease.readAll();
        lsbRelease.close();

        UbuntuVersion *ver = new UbuntuVersion;

        foreach(QString line, QString::fromLatin1(data).split(QLatin1String("\n"))) {
            if (line.startsWith(QLatin1String(Constants::DISTRIB_ID))) {
                ver->m_id = line.replace(QLatin1String(Constants::DISTRIB_ID),QLatin1String(""));

            } else if (line.startsWith(QLatin1String(Constants::DISTRIB_RELEASE))) {
                ver->m_release = line.replace(QLatin1String(Constants::DISTRIB_RELEASE),QLatin1String(""));

            } else if (line.startsWith(QLatin1String(Constants::DISTRIB_CODENAME))) {
                ver->m_codename = line.replace(QLatin1String(Constants::DISTRIB_CODENAME),QLatin1String(""));

            } else if (line.startsWith(QLatin1String(Constants::DISTRIB_DESCRIPTION))) {
                ver->m_description = line.replace(QLatin1String(Constants::DISTRIB_DESCRIPTION),QLatin1String(""));
            }
        }

        return ver;
    }
    return 0;
}
