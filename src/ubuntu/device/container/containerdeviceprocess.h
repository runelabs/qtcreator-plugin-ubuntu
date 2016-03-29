#ifndef UBUNTU_INTERNAL_CONTAINERDEVICEPROCESS_H
#define UBUNTU_INTERNAL_CONTAINERDEVICEPROCESS_H

#include "containerdevice.h"

#include <projectexplorer/devicesupport/deviceprocess.h>
#include <utils/environment.h>

namespace Ubuntu {
namespace Internal {

class ContainerDevice;

class ContainerDeviceProcess: public ProjectExplorer::DeviceProcess
{
    Q_OBJECT
public:
    ContainerDeviceProcess(const ContainerDevice::ConstPtr &device, QObject *parent = 0);

    void start(const QString &executable, const QStringList &arguments);
    void interrupt();
    void terminate();
    void kill();

    QProcess::ProcessState state() const;
    QProcess::ExitStatus exitStatus() const;
    int exitCode() const;
    QString errorString() const;

    Utils::Environment environment() const;
    void setEnvironment(const Utils::Environment &env);

    void setWorkingDirectory(const QString &directory);

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    qint64 write(const QByteArray &data);

protected:
    ContainerDevice::ConstPtr containerDevice() const;

private:
    QProcess * const m_process;
    Utils::Environment m_env;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CONTAINERDEVICEPROCESS_H
