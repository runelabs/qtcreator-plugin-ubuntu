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
        DetectionStateRole,
        KitListRole,
        DeveloperModeRole,
        NetworkConnectionRole,
        WriteableImageRole,
        DeveloperToolsRole
    };

    explicit UbuntuDevicesModel(QObject *parent = 0);

    int findDevice(int uniqueIdentifier) const;

    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QHash<int, QByteArray> roleNames() const;
signals:

public slots:

protected:
    bool hasDevice (int uniqueIdentifier) const;
    UbuntuDevicesItem *createItem (UbuntuDevice::Ptr dev);


protected slots:
    void readDevicesFromSettings();
    void deviceDataChanged ();
    void deviceAdded(const Core::Id &id);
    void deviceRemoved(const Core::Id &id);
    void deviceUpdated(const Core::Id &id);

private:
     QList<UbuntuDevicesItem*> m_knownDevices;
};

class UbuntuDevicesItem : public QObject
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

    UbuntuDevicesItem(Ubuntu::Internal::UbuntuDevice::Ptr device, QObject* parent = 0);

    Core::Id id() const;
    UbuntuDevice::Ptr device() const;
    QVariantList kits() const;

signals:
    void kitsChanged ();
    void connectionChanged ();
    void detectionStateChanged ();

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
