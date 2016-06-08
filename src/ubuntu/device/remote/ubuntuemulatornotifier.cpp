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
#include "ubuntuemulatornotifier.h"
#include <ubuntu/ubuntuconstants.h>

#include <QTimer>
#include <QRegularExpression>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuEmulatorNotifier
 * Polls adb if a specific
 */

UbuntuEmulatorNotifier::UbuntuEmulatorNotifier(QObject *parent) :
    IUbuntuDeviceNotifier(parent),
    m_connected(false)
{
    m_pollTimout  = new QTimer(this);
    m_pollProcess = new QProcess(this);

    connect(m_pollTimout,SIGNAL(timeout()),this,SLOT(pollTimeout()));
    connect(m_pollProcess,SIGNAL(readyRead()),this,SLOT(pollProcessReadyRead()));
    connect(m_pollProcess,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(pollProcessFinished(int,QProcess::ExitStatus)));
}

void UbuntuEmulatorNotifier::startMonitoring(const QString &imageName)
{
    m_imageName = imageName;
    m_pollTimout->setInterval(5000);
    m_pollTimout->start();
}

void UbuntuEmulatorNotifier::stopMonitoring()
{
    m_pollTimout->stop();
    m_pollProcess->kill();
}

bool UbuntuEmulatorNotifier::isConnected() const
{
    return m_connected;
}

void UbuntuEmulatorNotifier::pollTimeout()
{
    m_buffer.clear();
    m_pollTimout->stop();

    m_pollProcess->setWorkingDirectory(QStringLiteral("/tmp"));
    m_pollProcess->start(QStringLiteral("%1/local_emulator_pid").arg(Constants::UBUNTU_SCRIPTPATH),
                         QStringList()
                         <<m_imageName );
}

void UbuntuEmulatorNotifier::pollProcessReadyRead()
{
    m_buffer.append(m_pollProcess->readAllStandardOutput());
}

void UbuntuEmulatorNotifier::pollProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    m_pollTimout->start();
    pollProcessReadyRead();
    if (exitCode == 0) {
        if (!m_connected) {
            m_connected = true;
            emit deviceConnected();
        }
    } else {
        if (m_connected) {
            m_connected = false;
            emit deviceDisconnected();
        }
    }
}

} // namespace Internal
} // namespace Ubuntu
