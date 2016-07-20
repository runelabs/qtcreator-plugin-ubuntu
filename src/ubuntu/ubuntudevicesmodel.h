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

#ifndef UBUNTUDEVICESMODEL_H
#define UBUNTUDEVICESMODEL_H

#include <ubuntu/device/remote/ubuntudevice.h>
#include <ubuntu/device/remote/ubuntudevicenotifier.h>
#include "ubuntuprocess.h"
#include <coreplugin/id.h>

#include <QAbstractListModel>
#include <QList>
#include <QTime>

namespace ProjectExplorer {
class Kit;
}

namespace Ubuntu {
namespace Internal {

class UbuntuDevicesItem;

class UbuntuDevicesModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum Roles {
        ConnectionStateRole = Qt::UserRole,
        UniqueIdRole,
        ConnectionStateStringRole,
        DetectionStateRole,
        DetectionStateStringRole,
        KitListRole,
        DeveloperModeRole,
        NetworkConnectionRole,
        WriteableImageRole,
        DeveloperToolsRole,
        LogRole,
        SerialIdRole,
        ModelInfoRole,
        DeviceInfoRole,
        ProductInfoRole,
        MachineTypeRole,
        FrameworkRole,
        EmulatorImageRole,
        EmulatorDeviceVersionRole,
        EmulatorUbuntuVersionRole,
        EmulatorImageVersionRole,
        EmulatorScaleFactorRole,
        EmulatorMemorySettingRole
    };

    enum State {
        Initial,
        CheckEmulatorInstalled,
        InstallEmulator,
        CreateEmulatorImage,
        ReadFromSettings,
        FindImages,
        AdbList,
        Idle
    };

    Q_PROPERTY(bool emulatorInstalled READ emulatorInstalled WRITE setEmulatorInstalled NOTIFY emulatorInstalledChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool cancellable READ cancellable NOTIFY cancellableChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)

    explicit UbuntuDevicesModel(QObject *parent = 0);

    Q_INVOKABLE bool set(int index, const QString &role, const QVariant &value);

    int findDevice(const Core::Id &devId) const;
    bool hasDevice (const Core::Id &devId) const;
    UbuntuDevice::ConstPtr device ( const int index );

    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool cancellable() const;
    QString state() const;
    bool busy() const;
    bool emulatorInstalled() const;

    static void doCreateEmulatorImage ( UbuntuProcess *process, const QString &name, const QString &arch, const QString &channel, const QString &passwd );
signals:
    void logMessage (const QString &str);
    void stdOutMessage (const QString &str);
    void stdErrMessage (const QString &str);

    void busyChanged(bool arg);
    void emulatorInstalledChanged(bool arg);
    void cancellableChanged(bool arg);
    void stateChanged(QString arg);

public slots:
    void triggerPortForwarding  ( const QVariant &devId );
    void triggerSSHSetup        ( const QVariant &devId );
    void triggerSSHConnection   ( const QVariant &devId );
    void triggerKitAutocreate   ( const QVariant &devId );
    void triggerKitRemove       ( const QVariant &devId, const QVariant &kitid );
    void triggerRedetect        ( const QVariant &devId );
    void deleteDevice           ( const QVariant &devId );
    void createEmulatorImage    ( const QString &name, const QString &arch, const QString &channel, const QString &passwd );
    void startEmulator          ( const QString &name );
    void stopEmulator           ( const QString &name );
    void deleteEmulator         ( const QString &name );
    QVariant validateEmulatorName(const QString &name );
    void refresh                ();
    void cancel();

protected:
    void clear ();
    UbuntuDevicesItem *createItem (UbuntuDevice::Ptr dev);
    int indexFromHelper (QObject* possibleHelper);
    void deviceChanged(QObject* possibleHelper, const QVector<int> &relatedRoles);
    void registerNewDevice(const QString &serial, const QString &arch);

    void setState(UbuntuDevicesModel::State newState);
    void setBusy(bool arg);
    void setEmulatorInstalled(bool arg);
    void setCancellable(bool arg);

    void beginAction(const QString &msg);
    void endAction(const QString &msg);
    void checkEmulatorInstalled();
    void findEmulatorImages();
    void installEmulator();
    void queryAdb();

protected slots:
    void readDevicesFromSettings();
    void detectionStateChanged ();
    void featureDetected ();
    void deviceInfoUpdated();
    void logUpdated ();
    void kitsChanged ();
    void deviceAdded(const Core::Id &id);
    void deviceRemoved(const Core::Id &id);
    void deviceUpdated(const Core::Id &id);
    void deviceConnected(const QString&id);

    void onMessage(const QString &msg);
    void processFinished(const QString &, int exitCode);
private:
    QList<UbuntuDevicesItem*> m_knownDevices;
    UbuntuDeviceNotifier *m_deviceNotifier;
    bool      m_busy;
    bool      m_emulatorInstalled;
    bool      m_cancellable;
    State     m_state;

    QString   m_reply;
    UbuntuProcess *m_process;
    QTime m_profiler;
};

class UbuntuQmlFeatureState : public QObject
{
    Q_OBJECT
public:
    enum FeatureState {
        NotAvailable = UbuntuDevice::NotAvailable,
        Unknown = UbuntuDevice::Unknown,
        Available = UbuntuDevice::Available
    };
    Q_ENUMS(FeatureState)
};

class UbuntuQmlDeviceDetectionState : public QObject
{
    Q_OBJECT
public:
    enum DeviceDetectionState {
        NotStarted = UbuntuDevice::NotStarted,
        WaitForEmulator = UbuntuDevice::WaitForEmulatorStart,
        Booting = UbuntuDevice::WaitForBoot,
        Detecting,
        Done = UbuntuDevice::Done,
        Error = UbuntuDevice::Failed
    };
    Q_ENUMS(DeviceDetectionState)
};

class UbuntuQmlDeviceConnectionState : public QObject
{
    Q_OBJECT
public:
    enum DeviceConnectionState {
        ReadyToUse    = ProjectExplorer::IDevice::DeviceReadyToUse,
        Connected     = ProjectExplorer::IDevice::DeviceConnected,
        Disconnected  = ProjectExplorer::IDevice::DeviceDisconnected,
        StateUnknown  = ProjectExplorer::IDevice::DeviceStateUnknown
    };
    Q_ENUMS(DeviceConnectionState)
};

class UbuntuQmlDeviceMachineType : public QObject
{
    Q_OBJECT
public:
    enum MachineType {
        Hardware = ProjectExplorer::IDevice::Hardware,
        Emulator = ProjectExplorer::IDevice::Emulator
    };
    Q_ENUMS(MachineType)
};

class UbuntuDevicesItem : public QObject
{
    Q_OBJECT
public:
    UbuntuDevicesItem(Ubuntu::Internal::UbuntuDevice::Ptr device, QObject* parent = 0);

    Core::Id id() const;
    UbuntuDevice::Ptr device() const;
    QVariantList kits() const;

signals:
    void kitsChanged ();
    void connectionChanged ();
    void detectionStateChanged ();
    void featureDetected ();
    void logUpdated();
    void deviceInfoUpdated();

private slots:
    void onKitAdded(ProjectExplorer::Kit *k);
    void onKitRemoved(ProjectExplorer::Kit *k);
    void onKitUpdated(ProjectExplorer::Kit *k);
    void deviceStateChanged ();


private:
    Ubuntu::Internal::UbuntuDevice::Ptr m_device;
    QSet<Core::Id> m_myKits;
};

}
}



#endif // UBUNTUDEVICESMODEL_H
