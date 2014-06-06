#include "ubunturemoterunner.h"
#include "ubuntudevice.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <ssh/sshconnection.h>

#include <QProcess>
#include <QPointer>
#include <QRegularExpression>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 1
};

class UbuntuRemoteClickApplicationRunnerPrivate
{
public:

    UbuntuRemoteClickApplicationRunnerPrivate() :
        m_cppDebugPort(0),
        m_qmlDebugPort(0),
        m_launcherPid(-1)
    {

    }

    quint16 m_cppDebugPort;
    quint16 m_qmlDebugPort;
    Utils::Environment m_env;
    QPointer<QProcess> m_proc;
    UbuntuDevice::ConstPtr m_dev;

    QString m_launcherOutput;
    int m_launcherPid;
};

UbuntuRemoteClickApplicationRunner::UbuntuRemoteClickApplicationRunner(QObject *parent) :
    QObject(parent),
    d (new UbuntuRemoteClickApplicationRunnerPrivate())
{

}

UbuntuRemoteClickApplicationRunner::~UbuntuRemoteClickApplicationRunner()
{
    delete d;
}

void UbuntuRemoteClickApplicationRunner::start( UbuntuDevice::ConstPtr device, const QString &clickPackageName)
{
    QTC_ASSERT(d->m_proc.isNull(), return);
    QTC_ASSERT(device, return);

    d->m_dev  = device;
    d->m_proc = new QProcess(this);
    connect(d->m_proc,SIGNAL(finished(int)),this,SLOT(handleLauncherProcessFinished()));
    connect(d->m_proc,SIGNAL(readyReadStandardOutput()),this,SLOT(handleLauncherStdOut()));
    connect(d->m_proc,SIGNAL(readyReadStandardError()),this,SLOT(handleLauncherStdErr()));
    connect(d->m_proc,SIGNAL(error(QProcess::ProcessError)),this,SLOT(handleLauncherProcessError(QProcess::ProcessError)));


    QStringList args;
    args << QStringLiteral("/tmp/%1").arg(clickPackageName);

    Utils::Environment &env = d->m_env;
    Utils::Environment::const_iterator i = env.constBegin();
    for(;i!=env.constEnd();i++) {
        args << QStringLiteral("--env")
             << QStringLiteral("%1:%2").arg(i.key()).arg(i.value());
    }

    if (d->m_qmlDebugPort > 0)
        args.append(QStringLiteral("--qmldebug=port:%1,block").arg(d->m_qmlDebugPort));
    if (d->m_cppDebugPort > 0)
        args.append(QStringLiteral("--cppdebug=%1").arg(d->m_cppDebugPort));

    QString subCommand = QStringLiteral("cd /tmp && ./qtc_device_applaunch.py %1")
            .arg(Utils::QtcProcess::joinArgs(args));

    QString command = QStringLiteral("adb");
    QStringList adbArgs = QStringList()
            << QStringLiteral("-s")
            << d->m_dev->serialNumber()
            << QStringLiteral("shell")
            << QStringLiteral("sudo")
            << QStringLiteral("-i")
            << QStringLiteral("-u")
            << d->m_dev->sshParameters().userName
            << QStringLiteral("sh")
            << QStringLiteral("-c")
            << subCommand;

    if(debug) qDebug()<<"Starting application: "<<command<<Utils::QtcProcess::joinArgs(adbArgs);

    d->m_proc->setProgram(command);
    d->m_proc->setArguments(adbArgs);
    d->m_launcherOutput.clear();
    d->m_proc->start();
}

void UbuntuRemoteClickApplicationRunner::stop()
{
    if (d->m_launcherPid > 0) {
        int success = QProcess::execute(QStringLiteral("adb"), QStringList()
                          << QStringLiteral("-s")
                          << d->m_dev->serialNumber()
                          << QStringLiteral("shell")
                          << QStringLiteral("kill")
                          << QStringLiteral("-SIGINT")
                          << QString::number(d->m_launcherPid));

        if( success != 0 )
            emit reportError(tr("Could not stop the application"));
    }
}


quint16 UbuntuRemoteClickApplicationRunner::cppDebugPort() const
{
    return d->m_cppDebugPort;
}

void UbuntuRemoteClickApplicationRunner::setCppDebugPort(const quint16 &cppDebugPort)
{
    d->m_cppDebugPort = cppDebugPort;
}
quint16 UbuntuRemoteClickApplicationRunner::qmlDebugPort() const
{
    return d->m_qmlDebugPort;
}

void UbuntuRemoteClickApplicationRunner::setQmlDebugPort(const quint16 &qmlDebugPort)
{
    d->m_qmlDebugPort = qmlDebugPort;
}

Utils::Environment UbuntuRemoteClickApplicationRunner::env() const
{
    return d->m_env;
}

void UbuntuRemoteClickApplicationRunner::setEnv(const Utils::Environment &env)
{
    d->m_env = env;
}

void UbuntuRemoteClickApplicationRunner::cleanup()
{
    d->m_proc->disconnect(this);
    d->m_proc->deleteLater();
    d->m_proc.clear();
    d->m_launcherPid = -1;
    d->m_env.clear();
    d->m_dev.clear();
    d->m_cppDebugPort = 0;
    d->m_qmlDebugPort = 0;
}

void UbuntuRemoteClickApplicationRunner::handleLauncherProcessError(QProcess::ProcessError error)
{
    emit reportError(tr("Error launching the application: %1 %2").arg(error).arg(d->m_proc->errorString()));
    emit finished(false);
    cleanup();
}

void UbuntuRemoteClickApplicationRunner::handleLauncherProcessFinished()
{
    emit finished( d->m_proc->exitCode() == 0 );
    cleanup();
}

void UbuntuRemoteClickApplicationRunner::handleLauncherStdOut()
{
    QTC_ASSERT(!d->m_proc.isNull(),return);

    QByteArray output = d->m_proc->readAllStandardOutput();

    if (d->m_launcherPid <= 0) {
        d->m_launcherOutput.append( QString::fromUtf8(output) );
        QRegularExpression exp (QStringLiteral("Launcher PID: ([0-9]+)"));
        QRegularExpressionMatch match = exp.match(d->m_launcherOutput);
        if(match.hasMatch()) {
            bool ok = false;
            d->m_launcherPid = match.captured(1).toInt(&ok);
            //@TODO implement a flag that immediately stops the process if the user requested that before we had the PID
        }
    }

    emit launcherStdout(output);
}

void UbuntuRemoteClickApplicationRunner::handleLauncherStdErr()
{
    QTC_ASSERT(!d->m_proc.isNull(),return);
    emit launcherStderr(d->m_proc->readAllStandardError());
}

}
}
