#ifndef UBUNTU_INTERNAL_CONTAINERDEVICESIGNALOPERATION_H
#define UBUNTU_INTERNAL_CONTAINERDEVICESIGNALOPERATION_H

#include "containerdevice.h"

#include <QProcess>

namespace Ubuntu {
namespace Internal {

class ContainerDeviceSignalOperation : public ProjectExplorer::DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    ~ContainerDeviceSignalOperation();
    void killProcess(int pid);
    void killProcess(const QString &filePath);
    void interruptProcess(int pid);
    void interruptProcess(const QString &filePath);

private:
    void killProcessSilently(int pid);
    void interruptProcessSilently(int pid);

    void appendMsgCannotKill(int pid, const QString &why);
    void appendMsgCannotInterrupt(int pid, const QString &why);

protected:
    explicit ContainerDeviceSignalOperation(ContainerDevice::ConstPtr dev);

    friend class ContainerDevice;

    void sendSignal(int pid, int signal);
protected slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitState);
    void processError(QProcess::ProcessError procErr);

private:
    ContainerDevice::ConstPtr m_dev;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CONTAINERDEVICESIGNALOPERATION_H
