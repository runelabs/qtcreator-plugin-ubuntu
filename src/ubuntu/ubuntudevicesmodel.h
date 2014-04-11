#ifndef UBUNTUDEVICESMODEL_H
#define UBUNTUDEVICESMODEL_H

#include "ubuntudevice.h"

#include <QAbstractListModel>
#include <QList>

namespace Ubuntu {
namespace Internal {

class UbuntuDevicesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit UbuntuDevicesModel(QObject *parent = 0);

    int findDevice(int uniqueIdentifier) const;

    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QHash<int, QByteArray> roleNames() const;
signals:

public slots:

protected:
    bool hasDevice (int uniqueIdentifier) const;

protected slots:
    void readDevicesFromSettings();
    void deviceDataChanged ();
    void deviceAdded(const Core::Id &id);
    void deviceRemoved(const Core::Id &id);
    void deviceUpdated(const Core::Id &id);

private:
     QList<Ubuntu::Internal::UbuntuDevice::Ptr> m_knownDevices;
};

}
}



#endif // UBUNTUDEVICESMODEL_H
