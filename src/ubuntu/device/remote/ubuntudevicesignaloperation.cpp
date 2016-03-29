#include "ubuntudevicesignaloperation.h"
#include <ubuntu/ubuntuconstants.h>
#include <QProcess>
namespace Ubuntu {
namespace Internal {

UbuntuDeviceSignalOperation::UbuntuDeviceSignalOperation(UbuntuDevice::ConstPtr device)
    : m_device(device)
{

}

void UbuntuDeviceSignalOperation::killProcess(int pid)
{
    sendSignal(pid,9);
}

void UbuntuDeviceSignalOperation::killProcess(const QString &filePath)
{
    Q_UNUSED(filePath)
    emit finished(tr("Sending signals to processes by filePath is not supported on Ubuntu Devices"));
}

void UbuntuDeviceSignalOperation::interruptProcess(int pid)
{
    sendSignal(pid,2);
}

void UbuntuDeviceSignalOperation::interruptProcess(const QString &filePath)
{
    Q_UNUSED(filePath)
    emit finished(tr("Sending signals to processes by filePath is not supported on Ubuntu Devices"));
}

void UbuntuDeviceSignalOperation::sendSignal(int pid, int signal)
{
    QProcess *proc = new QProcess(this);

    if(!m_device) {
        emit finished(tr("There was a internal error when trying to kill the process"));
        return;
    }


    proc->setProgram(QStringLiteral("adb"));
    proc->setArguments(QStringList()
                       << QStringLiteral("-s")
                       << m_device->serialNumber()
                       << QStringLiteral("shell")
                       << QStringLiteral("kill -%1").arg(signal)
                       << QString::number(pid));

    connect(proc,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(proc,SIGNAL(finished(int,QProcess::ExitStatus)),proc,SLOT(deleteLater()));
    connect(proc,SIGNAL(error(QProcess::ProcessError)),this,SLOT(processError(QProcess::ProcessError)));

    proc->start();

}

void UbuntuDeviceSignalOperation::processFinished(int exitCode ,QProcess::ExitStatus exitState)
{
    if(exitCode == 0 && exitState == QProcess::NormalExit) {
        emit finished(QString());
        return;
    }

    QString error = QStringLiteral("Can not kill the process. Exit Code: %1").arg(exitCode);
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if(proc) {
        QString err;

        if(exitState != QProcess::NormalExit)
            err = proc->errorString();
        else
            err = QString::fromLatin1(proc->readAllStandardError());

        if(!err.isEmpty())
            error.append(QStringLiteral("\n%1").arg(err));
    }

    emit finished(error);
}

void UbuntuDeviceSignalOperation::processError(QProcess::ProcessError procErr)
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if(proc) {
        proc->deleteLater();
        QString error = QStringLiteral("Can not kill the process. Error: %1 %2").arg(procErr).arg(proc->errorString());
        emit finished(error);
    }
}

} // namespace Internal
} // namespace Ubuntu
