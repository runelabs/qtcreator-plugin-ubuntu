#include "containerdeviceprocess.h"
#include "../../ubuntuconstants.h"

#include <utils/qtcassert.h>

#include <QProcess>
#include <QDebug>

namespace Ubuntu {
namespace Internal {

ContainerDeviceProcess::ContainerDeviceProcess(const ContainerDevice::ConstPtr &device,
                                           QObject *parent)
    : DeviceProcess(device, parent), m_process(new QProcess(this))
{
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
            SIGNAL(error(QProcess::ProcessError)));
    connect(m_process, SIGNAL(finished(int)), SIGNAL(finished()));
    connect(m_process, SIGNAL(readyReadStandardOutput()), SIGNAL(readyReadStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), SIGNAL(readyReadStandardError()));
    connect(m_process, SIGNAL(started()), SIGNAL(started()));
}

void ContainerDeviceProcess::start(const QString &executable, const QStringList &arguments)
{
    QTC_ASSERT(m_process->state() == QProcess::NotRunning, return);

    QStringList args {
        QStringLiteral("run"),
        containerDevice()->containerName(),
        QStringLiteral("env")
    };
    for (auto it = m_env.constBegin(); it != m_env.constEnd(); ++it)
        args << QString(it.key()).append(QLatin1String("='")).append(it.value()).append(QLatin1String("\'"));

    args << executable;
    args << arguments;

    m_process->setProgram(QString::fromLatin1(Constants::UBUNTU_TARGET_TOOL).arg(Constants::UBUNTU_SCRIPTPATH));
    m_process->setArguments(args);
    m_process->start(executable, arguments);
}

void ContainerDeviceProcess::interrupt()
{
    qWarning()<<"Interrupting the process is not supported";
    terminate();
}

void ContainerDeviceProcess::terminate()
{
    m_process->terminate();
}

void ContainerDeviceProcess::kill()
{
    m_process->kill();
}

QProcess::ProcessState ContainerDeviceProcess::state() const
{
    return m_process->state();
}

QProcess::ExitStatus ContainerDeviceProcess::exitStatus() const
{
    return m_process->exitStatus();
}

int ContainerDeviceProcess::exitCode() const
{
    return m_process->exitCode();
}

QString ContainerDeviceProcess::errorString() const
{
    return m_process->errorString();
}

Utils::Environment ContainerDeviceProcess::environment() const
{
    return m_env;
}

void ContainerDeviceProcess::setEnvironment(const Utils::Environment &env)
{
    m_env = env;
}

void ContainerDeviceProcess::setWorkingDirectory(const QString &directory)
{
    m_process->setWorkingDirectory(directory);
}

QByteArray ContainerDeviceProcess::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

QByteArray ContainerDeviceProcess::readAllStandardError()
{
    return m_process->readAllStandardError();
}

qint64 ContainerDeviceProcess::write(const QByteArray &data)
{
    return m_process->write(data);
}

ContainerDevice::ConstPtr ContainerDeviceProcess::containerDevice() const
{
    return qSharedPointerCast<const ContainerDevice>(device());
}

} // namespace Internal
} // namespace Ubuntu

