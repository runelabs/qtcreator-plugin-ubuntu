#include "containerdeviceprocess.h"
#include "../../ubuntuconstants.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QProcess>
#include <QDebug>
#include <QUuid>

namespace Ubuntu {
namespace Internal {

ContainerDeviceProcess::ContainerDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device,
                                           QObject *parent)
    : LinuxDeviceProcess(device, parent)
{
    m_pidFile = QString::fromLatin1("/tmp/qtc.%1.pid").arg(QString::fromLatin1(QUuid::createUuid().toRfc4122().toHex()));
}

ContainerDeviceProcess::~ContainerDeviceProcess()
{
    SshDeviceProcess *cleaner = new SshDeviceProcess(device());

    auto callback = [this, cleaner](){
        if (cleaner->exitCode() != 0) {
            qWarning()<<"Cleaning the pidfile "<<m_pidFile<<" has failed";
        }
        cleaner->deleteLater();
    };

    connect(cleaner, &SshDeviceProcess::finished, callback);
    cleaner->start(QStringLiteral("rm"), QStringList{m_pidFile});
}

void ContainerDeviceProcess::setWorkingDirectory(const QString &directory)
{
    m_workingDir = directory;
    LinuxDeviceProcess::setWorkingDirectory(directory);
}

void ContainerDeviceProcess::doSignal(const int sig)
{
    SshDeviceProcess *signaler = new SshDeviceProcess(device(), this);
    connect(signaler, &SshDeviceProcess::finished, [signaler](){
        if (signaler->exitCode() != 0) {
            qWarning()<<"Killing the process has failed";
            qWarning()<<signaler->readAllStandardOutput();
            qWarning()<<signaler->readAllStandardError();
        }
        signaler->deleteLater();
    });
    QString cmd = QString::fromLatin1("kill -%2 `cat %1`").arg(m_pidFile).arg(sig);
    signaler->start(cmd, QStringList());
}

QString ContainerDeviceProcess::fullCommandLine() const
{
    QString fullCommandLine;
    QStringList rcFiles {
        QLatin1String("/etc/profile"),
        QLatin1String("$HOME/.profile")
    };
    foreach (const QString &filePath, rcFiles)
        fullCommandLine += QString::fromLatin1("test -f %1 && . %1;").arg(filePath);
    if (!m_workingDir.isEmpty()) {
        fullCommandLine.append(QLatin1String("cd ")).append(Utils::QtcProcess::quoteArgUnix(m_workingDir))
                .append(QLatin1String(" && "));
    }
    QString envString;
    for (auto it = environment().constBegin(); it != environment().constEnd(); ++it) {
        if (!envString.isEmpty())
            envString += QLatin1Char(' ');
        envString.append(it.key()).append(QLatin1String("='")).append(it.value())
                .append(QLatin1Char('\''));
    }
    if (!envString.isEmpty())
        fullCommandLine.append(QLatin1Char(' ')).append(envString);

    if (!fullCommandLine.isEmpty())
        fullCommandLine += QLatin1Char(' ');

    fullCommandLine.append(Utils::QtcProcess::quoteArgUnix(QStringLiteral("dbus-run-session")));
    fullCommandLine += QString::fromLatin1(" bash -c \"echo \\$\\$ > %1; exec ").arg(m_pidFile);
    fullCommandLine.append(Utils::QtcProcess::quoteArgUnix(executable()));
    if (!arguments().isEmpty()) {
        fullCommandLine.append(QLatin1Char(' '));
        fullCommandLine.append(Utils::QtcProcess::joinArgs(arguments(), Utils::OsTypeLinux));
    }
    fullCommandLine.append(QStringLiteral("\""));
    return fullCommandLine;
}

} // namespace Internal
} // namespace Ubuntu

