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

#include "ubuntubzr.h"
#include "ubuntuconstants.h"
#include <QCoreApplication>

using namespace Ubuntu::Internal;

UbuntuBzr::UbuntuBzr(QObject *parent) :
    QObject(parent), m_bInitialized(false)
{
    connect(&m_cmd,SIGNAL(finished(int)),this,SLOT(scriptExecuted(int)));
    m_cmd.setWorkingDirectory(QCoreApplication::applicationDirPath());
}

void UbuntuBzr::initialize() {
    m_cmd.start(QString(QLatin1String(Constants::UBUNTUBZR_INITIALIZE)).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH));
}

void UbuntuBzr::scriptExecuted(int sta) {
    QStringList data = QString(QString::fromLatin1(m_cmd.readAllStandardOutput())).trimmed().split(QLatin1String(Constants::LINEFEED));

    if (data.length()!=2) {
        return;
    }

    this->m_whoami = data.takeFirst();
    this->m_launchpadId = data.takeFirst();

    m_bInitialized = true;
    emit initializedChanged();
}
