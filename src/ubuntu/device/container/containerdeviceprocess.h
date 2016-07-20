#ifndef UBUNTU_INTERNAL_CONTAINERDEVICEPROCESS_H
#define UBUNTU_INTERNAL_CONTAINERDEVICEPROCESS_H

#include "containerdevice.h"

#include <remotelinux/linuxdeviceprocess.h>
#include <utils/environment.h>

namespace Ubuntu {
namespace Internal {

class ContainerDevice;

class ContainerDeviceProcess: public RemoteLinux::LinuxDeviceProcess
{
    Q_OBJECT
public:
    ContainerDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device, QObject *parent = 0);
    ~ContainerDeviceProcess();

    // DeviceProcess interface
    virtual void interrupt() override { doSignal(2); }
    virtual void terminate() override { doSignal(15); }
    virtual void kill() override { doSignal(9); }

    void doSignal (const int sig);
private:
    // SshDeviceProcess interface
    virtual QString fullCommandLine(const ProjectExplorer::StandardRunnable &) const override;
    QString m_pidFile;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CONTAINERDEVICEPROCESS_H
