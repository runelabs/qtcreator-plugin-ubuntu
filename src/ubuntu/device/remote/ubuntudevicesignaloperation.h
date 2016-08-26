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
    virtual void killProcess(qint64 pid) override;
    virtual void killProcess(const QString &filePath) override;
    virtual void interruptProcess(qint64 pid) override;
    virtual void interruptProcess(const QString &filePath) override;

private:
    void sendSignal (qint64 pid, int signal);

private slots:
    void processFinished (int exitCode, QProcess::ExitStatus exitState);
    void processError(QProcess::ProcessError procErr);

private:
    UbuntuDevice::ConstPtr m_device;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEVICESIGNALOPERATION_H
