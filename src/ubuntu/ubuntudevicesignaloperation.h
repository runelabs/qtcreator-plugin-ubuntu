#ifndef UBUNTU_INTERNAL_UBUNTUDEVICESIGNALOPERATION_H
#define UBUNTU_INTERNAL_UBUNTUDEVICESIGNALOPERATION_H

#include <projectexplorer/devicesupport/idevice.h>
#include "ubuntudevice.h"

namespace Ubuntu {
namespace Internal {

class UbuntuDeviceSignalOperation : public ProjectExplorer::DeviceProcessSignalOperation
{
    Q_OBJECT

public:
    UbuntuDeviceSignalOperation(UbuntuDevice::ConstPtr device);
    ~UbuntuDeviceSignalOperation() {}
    typedef QSharedPointer<UbuntuDeviceSignalOperation> Ptr;

    // DeviceProcessSignalOperation interface
    virtual void killProcess(int pid);
    virtual void killProcess(const QString &filePath);
    virtual void interruptProcess(int pid);
    virtual void interruptProcess(const QString &filePath);

private:
    void sendSignal (int pid, int signal);

private slots:
    void processFinished (int exitCode, QProcess::ExitStatus exitState);
    void processError(QProcess::ProcessError procErr);

private:
    UbuntuDevice::ConstPtr m_device;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEVICESIGNALOPERATION_H
