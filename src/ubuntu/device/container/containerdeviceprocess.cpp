#include "containerdeviceprocess.h"
#include "../../ubuntuconstants.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <projectexplorer/runnables.h>

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

    ProjectExplorer::StandardRunnable r;
    r.executable = QStringLiteral("rm");
    r.commandLineArguments = m_pidFile;
    cleaner->start(r);
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

    ProjectExplorer::StandardRunnable r;
    r.executable = QStringLiteral("kill");
    r.commandLineArguments = QString::fromLatin1("-%2 `cat %1`").arg(m_pidFile).arg(sig);
    signaler->start(r);
}

QString ContainerDeviceProcess::fullCommandLine(const ProjectExplorer::StandardRunnable &runnable) const
{
    QString fullCommandLine;
    QStringList rcFiles {
        QLatin1String("/etc/profile")
        , QLatin1String("$HOME/.profile")
    };
    foreach (const QString &filePath, rcFiles)
        fullCommandLine += QString::fromLatin1("test -f %1 && . %1;").arg(filePath);
    if (runnable.workingDirectory.isEmpty()) {
        fullCommandLine.append(QLatin1String("cd ")).append(Utils::QtcProcess::quoteArgUnix(runnable.workingDirectory))
                .append(QLatin1String(" && "));
    }
    QString envString;
    for (auto it = runnable.environment.constBegin(); it != runnable.environment.constEnd(); ++it) {
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
    fullCommandLine.append(Utils::QtcProcess::quoteArgUnix(runnable.executable));
    if (!runnable.commandLineArguments.isEmpty()) {
        fullCommandLine.append(QLatin1Char(' '));
        fullCommandLine.append(runnable.commandLineArguments);
    }
    fullCommandLine.append(QStringLiteral("\""));
    return fullCommandLine;
}

} // namespace Internal
} // namespace Ubuntu

