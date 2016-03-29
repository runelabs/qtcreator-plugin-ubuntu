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

#ifndef UBUNTUDEVICENOTIFIER_H
#define UBUNTUDEVICENOTIFIER_H

#include <QObject>
#include <libudev.h>
#include <QDebug>
#include <QSocketNotifier>

class IUbuntuDeviceNotifier : public QObject
{
    Q_OBJECT
public:
    IUbuntuDeviceNotifier (QObject *parent = 0);
    virtual void startMonitoring(const QString &serialNumber) = 0;
    virtual void stopMonitoring() = 0;
    virtual bool isConnected () const = 0;

signals:
    void deviceConnected();
    void deviceConnected(const QString &serialNumber);
    void deviceDisconnected();
};

class UbuntuDeviceNotifier : public IUbuntuDeviceNotifier
{
    Q_OBJECT

public:
    explicit UbuntuDeviceNotifier(QObject *parent = 0);
    ~UbuntuDeviceNotifier();

public slots:
    void startMonitoring(const QString &serialNumber) override;
    void stopMonitoring() override;
    bool isConnected () const override;

protected slots:
    void on_udev_event();

private:
    struct udev *m_dev;
    struct udev_monitor *m_udevMonitor;
    int m_udevMonitorFileDescriptor;
    QSocketNotifier* m_udevSocketNotifier;
    QString m_devNode;
    QString m_serialNumber;
};

#endif // UBUNTUDEVICENOTIFIER_H
