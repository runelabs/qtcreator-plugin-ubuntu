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
#ifndef UBUNTU_INTERNAL_UBUNTUEMULATORNOTIFIER_H
#define UBUNTU_INTERNAL_UBUNTUEMULATORNOTIFIER_H

#include "ubuntudevicenotifier.h"
#include <QProcess>

class QTimer;

namespace Ubuntu {
namespace Internal {

class UbuntuEmulatorNotifier : public IUbuntuDeviceNotifier
{
    Q_OBJECT

public:
    explicit UbuntuEmulatorNotifier(QObject *parent = 0);

    // IUbuntuDeviceNotifier interface
    virtual void startMonitoring(const QString &imageName) override;
    virtual void stopMonitoring() override;
    virtual bool isConnected() const override;

private slots:
    void pollTimeout ();
    void pollProcessReadyRead ();
    void pollProcessFinished  (int exitCode, QProcess::ExitStatus exitStatus);

private:
    QByteArray m_buffer;
    QString    m_imageName;
    QTimer    *m_pollTimout;
    QProcess  *m_pollProcess;
    bool       m_connected;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUEMULATORNOTIFIER_H
