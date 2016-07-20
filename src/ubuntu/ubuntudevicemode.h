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

#ifndef UBUNTUDEVICEMODE_H
#define UBUNTUDEVICEMODE_H

#include <ubuntu/device/remote/ubuntudevice.h>
#include <coreplugin/imode.h>

class QQuickView;

namespace Ubuntu {
namespace Internal {

class UbuntuDevicesModel;
class UbuntuEmulatorModel;
class UbuntuDeviceMode;

class UbuntuQMLDeviceMode : public QObject {
    Q_OBJECT

public:
    UbuntuQMLDeviceMode( UbuntuDeviceMode *parent );

    void showAddEmulatorDialog ();

public slots:
    void deviceSelected ( const QVariant index );
    void addText (const QString &arg);
    void addErrorText (const QString &error);

signals:
    void logChanged(const QString &arg);
    void appendText(const QString &newText);
    void openAddEmulatorDialog ();

private:
    UbuntuDeviceMode* m_mode;
};


class UbuntuDeviceMode : public Core::IMode
{
    Q_OBJECT

public:
    UbuntuDeviceMode(QObject *parent = 0);
    void initialize();

    static UbuntuDeviceMode *instance();
    UbuntuDevice::ConstPtr device();

    void deviceSelected ( const QVariant index );

public slots:
    void showAddEmulatorDialog ();

signals:
    void updateDeviceActions ();

protected:
    static UbuntuDeviceMode *m_instance;
    UbuntuDevicesModel  *m_devicesModel;
    UbuntuQMLDeviceMode *m_qmlControl;
    QQuickView *m_modeView;
    QWidget* m_modeWidget;
    QVariant m_deviceIndex;
    QString  m_log;
};


} // Internal
} // Ubuntu


#endif // UBUNTUDEVICEMODE_H
