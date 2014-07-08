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

UbuntuBzr *UbuntuBzr::m_instance = 0;

UbuntuBzr::UbuntuBzr() :
    m_bInitialized(false)
{
    Q_ASSERT(m_instance == 0);
    m_instance = this;

    connect(&m_cmd,SIGNAL(finished(int)),this,SLOT(scriptExecuted(int)));
    m_cmd.setWorkingDirectory(QCoreApplication::applicationDirPath());

    initialize();
}

UbuntuBzr::~UbuntuBzr()
{
    m_instance = 0;
}

UbuntuBzr *UbuntuBzr::instance()
{
    return m_instance;
}

void UbuntuBzr::initialize() {
    if(m_cmd.state() == QProcess::NotRunning)
        m_cmd.start(QString(QLatin1String(Constants::UBUNTUBZR_INITIALIZE)).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH));
}

bool UbuntuBzr::waitForFinished(int msecs)
{
    return m_cmd.waitForFinished(msecs);
}

void UbuntuBzr::scriptExecuted(int sta) {
    Q_UNUSED(sta);

    QStringList data = QString(QString::fromLocal8Bit(m_cmd.readAllStandardOutput())).trimmed().split(QLatin1String(Constants::LINEFEED));

    if (data.length()!=2) {
        return;
    }

    this->m_whoami = data.takeFirst();
    this->m_launchpadId = data.takeFirst();

    m_bInitialized = true;
    emit initializedChanged();
}
