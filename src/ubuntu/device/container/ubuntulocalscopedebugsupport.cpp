#include "ubuntulocalscopedebugsupport.h"
#include "ubuntulocalrunconfiguration.h"
#include <ubuntu/ubuntuprojecthelper.h>
#include <ubuntu/ubuntuconstants.h>

#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/abi.h>
#include <utils/qtcprocess.h>

#include <QTcpServer>
#include <QFileInfo>

namespace Ubuntu {
namespace Internal {

UbuntuLocalScopeDebugSupport::UbuntuLocalScopeDebugSupport(UbuntuLocalRunConfiguration *runConfig,
                                                           Debugger::DebuggerRunControl *runControl,
                                                           const QString &scopeRunnerPath)
    : QObject(runControl)
    , m_port(-1)
    , m_scopeRunnerPath(scopeRunnerPath)
    , m_runControl(runControl)
{
    m_executable = runConfig->executable();
    m_commandLineArguments = runConfig->commandLineArguments();

    ProjectExplorer::EnvironmentAspect *env = runConfig->extraAspect<ProjectExplorer::EnvironmentAspect>();
    if (env) {
        m_launcher.setEnvironment(env->environment());
    }

    connect (runControl, &Debugger::DebuggerRunControl::requestRemoteSetup,
             this, &UbuntuLocalScopeDebugSupport::handleRemoteSetupRequested);
    connect (&m_launcher,SIGNAL(appendMessage(QString,Utils::OutputFormat)),
             m_runControl, SLOT(appendMessage(QString,Utils::OutputFormat)));
    connect (&m_launcher, &ProjectExplorer::ApplicationLauncher::processStarted,
             this, &UbuntuLocalScopeDebugSupport::handleProcessStarted);
    connect (&m_launcher, &ProjectExplorer::ApplicationLauncher::processExited,
             this, &UbuntuLocalScopeDebugSupport::handleProcessExited);
    connect (&m_launcher, &ProjectExplorer::ApplicationLauncher::bringToForegroundRequested,
             m_runControl, &Debugger::DebuggerRunControl::bringApplicationToForeground);
    connect (&m_launcher, &ProjectExplorer::ApplicationLauncher::error,
             this, &UbuntuLocalScopeDebugSupport::handleError);
    connect (m_runControl, &Debugger::DebuggerRunControl::stateChanged,
             this, &UbuntuLocalScopeDebugSupport::handleStateChanged);

}

UbuntuLocalScopeDebugSupport::~UbuntuLocalScopeDebugSupport()
{
    m_launcher.stop();
}

void UbuntuLocalScopeDebugSupport::handleRemoteSetupRequested()
{
    // Inject the debug mode into the INI file, this is not perfect
    // as it will stick even for the next non debug run, and even though
    // we can then start without gdbserver the timouts will be always set
    // high, because the DebugMode=true setting is still there
    m_port = getLocalPort();

    QStringList args = Utils::QtcProcess::splitArgs(m_commandLineArguments);
    if (args.size() < 0) {
        Debugger::RemoteSetupResult res;
        res.success = false;
        res.reason = tr("Not enough arguments to run a scope.");
        m_runControl->notifyEngineRemoteSetupFinished(res);
        return;

    }

    QFileInfo iniFile(args.first());
    if (!iniFile.isFile()) {
        Debugger::RemoteSetupResult res;
        res.success = false;
        res.reason = tr("Ini filepath %1 is not valid.").arg(args.first());
        m_runControl->notifyEngineRemoteSetupFinished(res);
        return;
    }

    QString appId = iniFile.completeBaseName();

    //debughelper script name, source and destination path
    const QString debScript = QStringLiteral("qtc_device_debughelper.py");
    const QString debSourcePath = QStringLiteral("%1/%2").arg(Constants::UBUNTU_SCRIPTPATH).arg(debScript);

    QString commTemplate = QStringLiteral("%S scope %1 %C")
            .arg(appId); //tell our script the appid

    QString defaultSubCmd = QStringLiteral("%1 %R %S ")
            .arg(m_scopeRunnerPath);


    bool injected = UbuntuProjectHelper::injectScopeDebugHelper(
                        iniFile.absoluteFilePath(),
                        debSourcePath,
                        commTemplate,
                        defaultSubCmd);
    if (!injected) {
        Debugger::RemoteSetupResult res;
        res.success = false;
        res.reason = tr("Could not inject the debug helper");
        m_runControl->notifyEngineRemoteSetupFinished(res);
        return;
    }

    args.append(QStringLiteral("--cppdebug"));
    args.append(QString::number(m_port));
    m_launcher.start(m_appLauncherMode,
                     m_executable,
                     Utils::QtcProcess::joinArgs(args));
}

void UbuntuLocalScopeDebugSupport::handleProcessStarted()
{
    Debugger::RemoteSetupResult res;
    res.success = true;
    res.gdbServerPort = m_port;
    m_runControl->notifyEngineRemoteSetupFinished(res);
}

void UbuntuLocalScopeDebugSupport::handleProcessExited(int exitCode, QProcess::ExitStatus)
{
    if (exitCode != 0)
        m_runControl->notifyInferiorIll();
    else
        m_runControl->debuggingFinished();
}

void UbuntuLocalScopeDebugSupport::handleError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        Debugger::RemoteSetupResult res;
        res.success = false;
        res.reason = tr("The process failed to start");
        m_runControl->notifyEngineRemoteSetupFinished(res);
    }

    if (error == QProcess::Crashed)
        m_runControl->notifyInferiorIll();
}

void UbuntuLocalScopeDebugSupport::handleStateChanged(Debugger::DebuggerState state)
{
    qDebug() << "Changed to State: "<<state;
    if (state == Debugger::DebuggerFinished && m_launcher.isRunning())
        m_launcher.stop();
}

ProjectExplorer::ApplicationLauncher::Mode UbuntuLocalScopeDebugSupport::appLauncherMode() const
{
    return m_appLauncherMode;
}

void UbuntuLocalScopeDebugSupport::setAppLauncherMode(const ProjectExplorer::ApplicationLauncher::Mode &appLauncherMode)
{
    m_appLauncherMode = appLauncherMode;
}

quint16 UbuntuLocalScopeDebugSupport::getLocalPort() const
{
    QTcpServer srv;
    if (srv.listen(QHostAddress::LocalHost) || srv.listen(QHostAddress::LocalHostIPv6))
        return srv.serverPort();
    return -1;
}


} // namespace Internal
} // namespace Ubuntu
