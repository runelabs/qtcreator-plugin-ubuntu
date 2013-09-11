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

#include "ubuntudevicenotifier.h"

UbuntuDeviceNotifier::UbuntuDeviceNotifier(QObject *parent) :
    QObject(parent)
{
    m_dev = udev_new();
    m_udevMonitor = NULL;

    m_udevMonitor = udev_monitor_new_from_netlink(m_dev,"udev");
    if (!m_udevMonitor) {
        qWarning() << QLatin1String("could not monitor devices");
        return;
    }
    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor,"usb",0);
    udev_monitor_enable_receiving(m_udevMonitor);
    m_udevMonitorFileDescriptor = udev_monitor_get_fd(m_udevMonitor);
    m_udevSocketNotifier = new QSocketNotifier(m_udevMonitorFileDescriptor,QSocketNotifier::Read, this);

    connect(m_udevSocketNotifier,SIGNAL(activated(int)),this,SLOT(on_udev_event()));

}

UbuntuDeviceNotifier::~UbuntuDeviceNotifier() {
    m_udevSocketNotifier->disconnect(SIGNAL(activated(int)));
    m_udevSocketNotifier->deleteLater();
    udev_monitor_unref(m_udevMonitor);
    udev_unref(m_dev);
}

void UbuntuDeviceNotifier::startMonitoring(QString serialNumber) {
    m_serialNumber = serialNumber;

    // check if the device is connected or disconnected at the moment
    struct udev_list_entry *dev_list_entry;
    struct udev_device *dev;
    struct udev_enumerate *enumerate = udev_enumerate_new(m_dev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_add_match_sysattr(enumerate,"serial",serialNumber.toLatin1().constData());

    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(m_dev, path);
        m_devNode = QLatin1String(udev_device_get_devnode(dev));
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
}

void UbuntuDeviceNotifier::stopMonitoring() {
    m_serialNumber = QLatin1String("");
}

void UbuntuDeviceNotifier::on_udev_event() {
    if (!m_udevMonitor) {
        qDebug() << QLatin1String("no monitor");
        return;
    }

    struct udev_device *dev;
    dev = udev_monitor_receive_device(m_udevMonitor);
    if (!dev) {
        qDebug() << QLatin1String("no device");
        udev_device_unref(dev);
        return;
    }
    QString serial = QLatin1String(udev_device_get_sysattr_value(dev,"serial"));
    QString action = QLatin1String(udev_device_get_action(dev));
    QString devNode = QLatin1String(udev_device_get_devnode(dev));

    udev_device_unref(dev);

    if (action == QLatin1String("remove") && (m_devNode == devNode) && m_devNode.isEmpty()==false) {
        m_devNode = QLatin1String("");
        emit deviceDisconnected();
    } else if (action == QLatin1String("add") && m_serialNumber == serial && m_serialNumber.isEmpty()==false) {
        emit deviceConnected(m_serialNumber);
        m_devNode = devNode;
    } else if (action == QLatin1String("add")) {
        if (!serial.isEmpty()) {
            emit deviceConnected(serial);
        }
    }


}
