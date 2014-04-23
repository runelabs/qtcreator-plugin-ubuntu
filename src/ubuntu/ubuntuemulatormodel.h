#ifndef UBUNTU_INTERNAL_UBUNTUEMULATORMODEL_H
#define UBUNTU_INTERNAL_UBUNTUEMULATORMODEL_H

#include "ubuntuprocess.h"
#include <QAbstractListModel>

namespace Ubuntu {
namespace Internal {

class UbuntuEmulatorModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum State {
        Initial,
        CheckEmulator,
        InstallEmulator,
        CreateEmulatorImage,
        FindImages,
        Idle
    };

    enum Roles {
        UbuntuVersionRole = Qt::UserRole,
        DeviceVersionRole,
        ImageVersionRole
    };

    struct EmulatorItem {
        QString name;
        QString ubuntuVersion;
        QString deviceVersion;
        QString imageVersion;
    };

    Q_PROPERTY(bool emulatorInstalled READ emulatorInstalled WRITE setEmulatorInstalled NOTIFY emulatorInstalledChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)

    explicit UbuntuEmulatorModel(QObject *parent = 0);

    void setEmulatorInstalled(bool arg);
    bool emulatorInstalled() const;

    bool busy() const;
    QString state() const;

    // QAbstractItemModel interface
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QHash<int, QByteArray> roleNames() const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

protected:
    void clear();
    void setBusy(bool arg);
    void setState(State newState);

public slots:
    void checkEmulatorInstalled ();
    void findEmulatorImages     ();
    void installEmulator ();
    void createEmulatorImage (const QString &name);
protected slots:
    void onMessage (const QString &msg);
    void processFinished (const QString &, int);

signals:
    void emulatorInstalledChanged(bool arg);
    void busyChanged(bool arg);
    void stateChanged(const QString &arg);

private:
    QString m_reply;
    UbuntuProcess *m_process;
    bool m_emulatorInstalled;
    State m_state;
    bool m_busy;

    QList<EmulatorItem> m_data;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUEMULATORMODEL_H
