#ifndef UBUNTU_INTERNAL_UBUNTUEMULATORNOTIFIER_H
#define UBUNTU_INTERNAL_UBUNTUEMULATORNOTIFIER_H

#include "ubuntudevicenotifier.h"
#include <QProcess>

class QTimer;

namespace Ubuntu {
namespace Internal {

class UbuntuEmulatorNotifier : public IUbuntuDeviceNotifier
{
    Q_OBJECT
public:
    explicit UbuntuEmulatorNotifier(QObject *parent = 0);

    // IUbuntuDeviceNotifier interface
    virtual void startMonitoring(const QString &serialNumber);
    virtual void stopMonitoring();
    virtual bool isConnected() const;

private slots:
    void pollTimeout ();
    void pollProcessReadyRead ();
    void pollProcessFinished  (int exitCode, QProcess::ExitStatus exitStatus);

private:
    QByteArray m_buffer;
    QString    m_lastState;
    QString    m_serial;
    QTimer    *m_pollTimout;
    QProcess  *m_pollProcess;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUEMULATORNOTIFIER_H
