#include "ubuntulocalscopedebugsupport.h"
#include "ubuntulocalrunconfiguration.h"
#include <ubuntu/ubuntuprojecthelper.h>
#include <ubuntu/ubuntuconstants.h>

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/abi.h>
#include <utils/qtcprocess.h>

#include <QTcpServer>
#include <QFileInfo>

namespace Ubuntu {
namespace Internal {

UbuntuLocalScopeDebugSupport::UbuntuLocalScopeDebugSupport(UbuntuLocalRunConfiguration *runConfig,
                                                           Debugger::DebuggerRunControl *runControl,
                                                           const QString &scopeRunnerPath) :
    RemoteLinux::AbstractRemoteLinuxRunSupport(runConfig, runControl)
    , m_port(-1)
    , m_scopeRunnerPath(scopeRunnerPath)
    , m_runControl(runControl)
{
    m_executable = runConfig->remoteExecutableFilePath();
    m_commandLineArguments = runConfig->arguments();

    connect (runControl, &Debugger::DebuggerRunControl::requestRemoteSetup,
             this, &UbuntuLocalScopeDebugSupport::handleRemoteSetupRequested);

}

UbuntuLocalScopeDebugSupport::~UbuntuLocalScopeDebugSupport()
{
}

void UbuntuLocalScopeDebugSupport::startExecution()
{
    QTC_ASSERT(state() == GatheringPorts, return);

    setState(StartingRunner);
    m_gdbserverOutput.clear();

    // Inject the debug mode into the INI file, this is not perfect
    // as it will stick even for the next non debug run, and even though
    // we can then start without gdbserver the timouts will be always set
    // high, because the DebugMode=true setting is still there
    if(!setPort(m_port)) {
        Debugger::RemoteSetupResult res;
        res.success = false;
        res.reason = tr("Could not assign a free port for debugging.");
        m_runControl->notifyEngineRemoteSetupFinished(res);
        return;

    }

    QStringList args = m_commandLineArguments;
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

    Utils::FileName debSourcePath = Utils::FileName::fromString(Constants::UBUNTU_SCRIPTPATH);
    debSourcePath.appendPath(debScript);

    Utils::FileName debTargetPath = Utils::FileName::fromString(iniFile.absolutePath());
    debTargetPath.appendPath(debScript);

    if (QFile::exists(debTargetPath.toString()))
        QFile::remove(debTargetPath.toString());
    if (!QFile::copy(debSourcePath.toString(), debTargetPath.toString())) {
        Debugger::RemoteSetupResult res;
        res.success = false;
        res.reason = tr("Unable to copy the debug helper script to: %1.").arg(iniFile.absolutePath());
        m_runControl->notifyEngineRemoteSetupFinished(res);
        return;
    }


    QString commTemplate = QStringLiteral("%S scope %1 %C")
            .arg(appId); //tell our script the appid

    QString defaultSubCmd = QStringLiteral("%1 %R %S ")
            .arg(m_scopeRunnerPath);


    bool injected = UbuntuProjectHelper::injectScopeDebugHelper(
                        iniFile.absoluteFilePath(),
                        debTargetPath.toString(),
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


    setState(StartingRunner);
    m_gdbserverOutput.clear();

    ProjectExplorer::DeviceApplicationRunner *runner = appRunner();
    connect(runner, &ProjectExplorer::DeviceApplicationRunner::remoteStderr,
            this, &UbuntuLocalScopeDebugSupport::handleRemoteErrorOutput);
    connect(runner, &ProjectExplorer::DeviceApplicationRunner::remoteStdout,
            this, &UbuntuLocalScopeDebugSupport::handleRemoteOutput);
    connect(runner, &ProjectExplorer::DeviceApplicationRunner::finished,
            this, &UbuntuLocalScopeDebugSupport::handleAppRunnerFinished);
    connect(runner, &ProjectExplorer::DeviceApplicationRunner::reportProgress,
            this, &UbuntuLocalScopeDebugSupport::handleProgressReport);
    connect(runner, &ProjectExplorer::DeviceApplicationRunner::reportError,
            this, &UbuntuLocalScopeDebugSupport::handleAppRunnerError);
    connect(m_runControl, &Debugger::DebuggerRunControl::stateChanged,
            this, &UbuntuLocalScopeDebugSupport::handleStateChanged);

    runner->setEnvironment(environment());
    runner->setWorkingDirectory(workingDirectory());
    runner->start(device(), m_executable, args);
}

void UbuntuLocalScopeDebugSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), Debugger::LogStatus);
    AbstractRemoteLinuxRunSupport::handleRemoteSetupRequested();
}

void UbuntuLocalScopeDebugSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, Debugger::AppError);
        if (m_runControl)
            m_runControl->notifyInferiorIll();
    } else if (state() != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void UbuntuLocalScopeDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Inactive || state() == Running || state() == StartingRunner, return);

    showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void UbuntuLocalScopeDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != GatheringPorts, return);

    if (!m_runControl)
        return;

    showMessage(QString::fromUtf8(output), Debugger::AppError);
    if (state() == StartingRunner) {
        m_gdbserverOutput += output;
        if (m_gdbserverOutput.contains("Listening on port")) {
            handleAdapterSetupDone();
            m_gdbserverOutput.clear();
        }
    }
}

void UbuntuLocalScopeDebugSupport::handleAppRunnerFinished(bool success)
{
    if (!m_runControl || state() == Inactive)
        return;

    if (state() == Running) {
        if (!success)
            m_runControl->notifyInferiorIll();
    } else if (state() == StartingRunner) {
        Debugger::RemoteSetupResult result;
        result.success = false;
        result.reason = tr("Debugging failed.");
        m_runControl->notifyEngineRemoteSetupFinished(result);
    }
}

void UbuntuLocalScopeDebugSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), Debugger::LogStatus);
}

void UbuntuLocalScopeDebugSupport::showMessage(const QString &msg, int channel)
{
    if (state() != Inactive && m_runControl)
        m_runControl->showMessage(msg, channel);
}

void UbuntuLocalScopeDebugSupport::handleAdapterSetupDone()
{
    AbstractRemoteLinuxRunSupport::handleAdapterSetupDone();

    Debugger::RemoteSetupResult result;
    result.success = true;
    result.gdbServerPort = m_port;
    m_runControl->notifyEngineRemoteSetupFinished(result);
}

void UbuntuLocalScopeDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(state() == StartingRunner, return);
    handleAdapterSetupDone();
}

void UbuntuLocalScopeDebugSupport::handleStateChanged(Debugger::DebuggerState state)
{
    if (state == Debugger::DebuggerFinished) {
        setFinished();
    }
}

} // namespace Internal
} // namespace Ubuntu
