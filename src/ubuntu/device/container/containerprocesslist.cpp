#include "containerprocesslist.h"
#include "containerdevice.h"
#include "../../ubuntuconstants.h"

#include <utils/synchronousprocess.h>
#include <coreplugin/id.h>

#include <QProcess>
#include <QTimer>


namespace Ubuntu {
namespace Internal {

// shamelessly stolen from localprocesslist.cpp
// Determine UNIX processes by running ps
static QList<ProjectExplorer::DeviceProcessItem> getContainerProcessesWithPs(const QString &containerName)
{
    QList<ProjectExplorer::DeviceProcessItem> processes;
    QProcess psProcess;
    QStringList args;
    args <<QStringLiteral("run") << containerName << QStringLiteral("ps")
        << QStringLiteral("-e") << QStringLiteral("-o") << QStringLiteral("pid,comm,args");
    psProcess.start(QString::fromLatin1(Constants::UBUNTU_TARGET_TOOL).arg(Constants::UBUNTU_SCRIPTPATH), args);
    if (psProcess.waitForStarted()) {
        QByteArray output;
        if (Utils::SynchronousProcess::readDataFromProcess(psProcess, 30000, &output, 0, false)) {
            // Split "457 /Users/foo.app arg1 arg2"
            const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
            const int lineCount = lines.size();
            const QChar blank = QLatin1Char(' ');
            for (int l = 1; l < lineCount; l++) { // Skip header
                const QString line = lines.at(l).simplified();
                const int pidSep = line.indexOf(blank);
                const int cmdSep = pidSep != -1 ? line.indexOf(blank, pidSep + 1) : -1;
                if (cmdSep > 0) {
                    const int argsSep = cmdSep != -1 ? line.indexOf(blank, cmdSep + 1) : -1;
                    ProjectExplorer::DeviceProcessItem procData;
                    procData.pid = line.left(pidSep).toInt();
                    procData.cmdLine = line.mid(cmdSep + 1);
                    if (argsSep == -1)
                        procData.exe = line.mid(cmdSep + 1);
                    else
                        procData.exe = line.mid(cmdSep + 1, argsSep - cmdSep -1);
                    processes.push_back(procData);
                }
            }
        }
    }
    return processes;
}

ContainerProcessList::ContainerProcessList(const ContainerDevice::ConstPtr &device, QObject *parent)
    : DeviceProcessList(device, parent)
{

}

QList<ProjectExplorer::DeviceProcessItem> ContainerProcessList::getContainerProcesses(const QString &container)
{
    return getContainerProcessesWithPs(container);
}

void ContainerProcessList::doKillProcess(const ProjectExplorer::DeviceProcessItem &process)
{
    m_sig = device()->signalOperation();
    connect(m_sig.data(), SIGNAL(finished(QString)),
            SLOT(reportDelayedKillStatus(QString)));
    m_sig->killProcess(process.pid);
}

void ContainerProcessList::handleUpdate()
{
    ContainerDevice::ConstPtr dev = qSharedPointerCast<const ContainerDevice>(device());
    reportProcessListUpdated(getContainerProcesses(dev->containerName()));
}

void ContainerProcessList::doUpdate()
{
    QTimer::singleShot(0, this, SLOT(handleUpdate()));
}

void ContainerProcessList::reportDelayedKillStatus(const QString &errorMessage)
{
    if (errorMessage.isEmpty())
        reportProcessKilled();
    else
        reportError(errorMessage);

    m_sig.clear();
}

} // namespace Internal
} // namespace Ubuntu

