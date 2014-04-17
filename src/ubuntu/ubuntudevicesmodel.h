#ifndef UBUNTUDEVICESMODEL_H
#define UBUNTUDEVICESMODEL_H

#include "ubuntudevice.h"

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
        LogRole
    };

    explicit UbuntuDevicesModel(QObject *parent = 0);

    Q_INVOKABLE bool set(int index, const QString &role, const QVariant &value);

    int findDevice(int uniqueIdentifier) const;


    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QHash<int, QByteArray> roleNames() const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
signals:

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

protected:
    bool hasDevice (int uniqueIdentifier) const;
    UbuntuDevicesItem *createItem (UbuntuDevice::Ptr dev);
    int indexFromHelper (QObject* possibleHelper);
    void deviceChanged(QObject* possibleHelper, const QVector<int> &relatedRoles);


protected slots:
    void readDevicesFromSettings();
    void detectionStateChanged ();
    void featureDetected ();
    void logUpdated ();
    void kitsChanged ();
    void deviceAdded(const Core::Id &id);
    void deviceRemoved(const Core::Id &id);
    void deviceUpdated(const Core::Id &id);

private:
     QList<UbuntuDevicesItem*> m_knownDevices;
};

class UbuntuDeviceStates : public QObject
{
    Q_OBJECT
public:
    enum FeatureState {
        NotAvailable = UbuntuDevice::NotAvailable,
        Unknown = UbuntuDevice::Unknown,
        Available = UbuntuDevice::Available
    };
    Q_ENUMS(FeatureState)

    enum DeviceDetectionState {
        NotStarted = UbuntuDevice::NotStarted,
        Detecting,
        Done = UbuntuDevice::Done
    };
    Q_ENUMS(DeviceDetectionState)

    enum DeviceConnectionState {
        DeviceReadyToUse    = ProjectExplorer::IDevice::DeviceReadyToUse,
        DeviceConnected     = ProjectExplorer::IDevice::DeviceConnected,
        DeviceDisconnected  = ProjectExplorer::IDevice::DeviceDisconnected,
        DeviceStateUnknown  = ProjectExplorer::IDevice::DeviceStateUnknown
    };
    Q_ENUMS(DeviceConnectionState)
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
