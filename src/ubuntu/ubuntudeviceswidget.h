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

#ifndef UBUNTUDEVICESWIDGET_H
#define UBUNTUDEVICESWIDGET_H

#include <QWidget>
#include <QListWidget>
#include "ubuntudevicenotifier.h"
#include "ubuntuprocess.h"
#include "ubuntudevice.h"

namespace Ui {
class UbuntuDevicesWidget;
}

class UbuntuDevicesWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit UbuntuDevicesWidget(QWidget *parent = 0);
    ~UbuntuDevicesWidget();

    static UbuntuDevicesWidget* instance();

    bool deviceDetected() { return m_deviceDetected; }
    QString serialNumber();

signals:
    void updateDeviceActions();
    
protected slots:
    void onMessage(QString msg);
    void onFinished(QString cmd, int code);
    void onError(QString msg);
    void onStarted(QString cmd);

    void on_pushButton_InstallEmulator_OK_clicked();
    void on_pushButton_CreateNewEmulator_clicked();
    void on_pushButton_StartEmulator_clicked();

    void onDeviceConnected(QString serialNumber);

    void on_pushButtonRefresh_clicked();
    void on_pushButtonRefresh_2_clicked() { on_pushButtonRefresh_clicked(); }
    void on_comboBoxSerialNumber_currentIndexChanged( const QString & text );

    void checkEmulator();
    void checkEmulatorInstances();
    void detectDevices();

private slots:
    void readDevicesFromSettings();
    void deviceAdded (const Core::Id& id);
    void deviceRemoved (const Core::Id& id);
    void knownDeviceConnected ();
    void knownDeviceDisconnected ();
    void slotChanged();
    void startEmulator(QListWidgetItem*);

private:
    void beginAction(QString);
    void endAction(QString);
    int  addDevice(Ubuntu::Internal::UbuntuDevice* dev);
    void removeDevice(Ubuntu::Internal::UbuntuDevice* dev);

    QMap<int,Ubuntu::Internal::UbuntuDevice::Ptr> m_knownDevices;


    Ui::UbuntuDevicesWidget *ui;

    Ubuntu::Internal::UbuntuProcess m_ubuntuProcess;
    QString m_reply;

    UbuntuDeviceNotifier m_ubuntuDeviceNotifier;

    bool m_aboutToClose;
    bool m_deviceDetected;
    QString m_deviceSerialNumber;

    static UbuntuDevicesWidget *m_instance;

    bool validate();
};


#endif // UBUNTUDEVICESWIDGET_H
