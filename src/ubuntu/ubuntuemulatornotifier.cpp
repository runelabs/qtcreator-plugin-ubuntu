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
    m_currentOperation(None),
    m_currentState(Disconnected)
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

    if (m_currentState == Disconnected) {

    } else {
        m_pollProcess->start(QLatin1String("adb"),
                             QStringList()
                             <<QLatin1String("-s")
                             <<m_imageName
                             <<QLatin1String("get-state"));
    }
}

void UbuntuEmulatorNotifier::pollProcessReadyRead()
{
    m_buffer.append(m_pollProcess->readAllStandardOutput());
}

void UbuntuEmulatorNotifier::pollProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {

        if (m_currentState == Connected) {
            QString str = QString::fromLocal8Bit(m_buffer.data());

            QRegularExpression expr(QLatin1String("(device|unknown|offline|bootloader)"));
            QRegularExpressionMatch match = expr.match(str);
            if(match.hasMatch()) {
                bool wasConnected = isConnected();
                QString newState = match.captured(1);
                if(newState != m_currentState) {
                    m_currentState = newState;
                    if(isConnected() && !wasConnected)
                        emit deviceConnected();
                    else if(!isConnected() && wasConnected)
                        emit deviceDisconnected();
                }
            }
        } else {

        }
    }
}

} // namespace Internal
} // namespace Ubuntu
