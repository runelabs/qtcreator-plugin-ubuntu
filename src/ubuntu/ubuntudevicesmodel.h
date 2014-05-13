#ifndef UBUNTUDEVICESMODEL_H
#define UBUNTUDEVICESMODEL_H

#include "ubuntudevice.h"
#include "ubuntudevicenotifier.h"
#include "ubuntuprocess.h"
#include <coreplugin/id.h>

#include <QAbstractListModel>
#include <QList>

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
        EmulatorRole,
        LogRole,
        SerialIdRole,
        ModelInfoRole,
        DeviceInfoRole,
        ProductInfoRole,
        MachineTypeRole
    };

    enum State {
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

    int findDevice(int uniqueIdentifier) const;
    bool hasDevice (int uniqueIdentifier) const;
    UbuntuDevice::ConstPtr device ( const int index );

    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QHash<int, QByteArray> roleNames() const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    bool cancellable() const;
    QString state() const;
    bool busy() const;
    bool emulatorInstalled() const;
signals:
    void logMessage (const QString &str);
    void stdOutMessage (const QString &str);
    void stdErrMessage (const QString &str);

    void busyChanged(bool arg);
    void emulatorInstalledChanged(bool arg);
    void cancellableChanged(bool arg);
    void stateChanged(QString arg);

public slots:
    void triggerCloneTimeConfig ( const int devId );
    void triggerPortForwarding  ( const int devId );
    void triggerSSHSetup        ( const int devId );
    void triggerSSHConnection   ( const int devId );
    void triggerReboot          ( const int devId );
    void triggerRebootBootloader( const int devId );
    void triggerRebootRecovery  ( const int devId );
    void triggerShutdown        ( const int devId );
    void triggerKitAutocreate   ( const int devId );
    void triggerKitRemove       (const int devId, const QVariant &kitid );
    void startEmulator          (const QString &name);
    QVariant validateEmulatorName(const QString &name);
    void refresh                ();
    void cancel();

protected:
    void clear ();
    UbuntuDevicesItem *createItem (UbuntuDevice::Ptr dev);
    int indexFromHelper (QObject* possibleHelper);
    void deviceChanged(QObject* possibleHelper, const QVector<int> &relatedRoles);
    void registerNewDevice(const QString &serial, const QString &deviceInfo);

    void setState(UbuntuDevicesModel::State newState);
    void setBusy(bool arg);
    void setEmulatorInstalled(bool arg);
    void setCancellable(bool arg);

    void beginAction(const QString &msg);
    void endAction(const QString &msg);
    void checkEmulatorInstalled();
    void findEmulatorImages();
    void installEmulator();
    void createEmulatorImage(const QString &name);
    void queryAdb();

protected slots:
    void readDevicesFromSettings();
    void detectionStateChanged ();
    void featureDetected ();
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
        Booting = UbuntuDevice::WaitForBoot,
        Detecting,
        Done = UbuntuDevice::Done
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
