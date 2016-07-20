#include "ubunturemoteanalyzesupport.h"
#include "ubunturemoterunconfiguration.h"
#include "ubunturemoterunner.h"
#include <ubuntu/ubuntuconstants.h>

#include <remotelinux/remotelinuxrunconfiguration.h>
#include <debugger/analyzer/analyzerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>

#include <utils/qtcassert.h>
#include <utils/port.h>
#include <qmldebug/qmloutputparser.h>

#include <QPointer>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteAnalyzeSupportPrivate
{
public:
    UbuntuRemoteAnalyzeSupportPrivate(Debugger::AnalyzerRunControl *rc, Core::Id runMode)
        : runControl(rc),
          qmlPort(-1),
          qmlProfiling(runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
    {
    }

    QString clickPackage;

    const QPointer<Debugger::AnalyzerRunControl> runControl;
    QmlDebug::QmlOutputParser outputParser;

    Utils::Port qmlPort;
    bool qmlProfiling;
};

UbuntuRemoteAnalyzeSupport::UbuntuRemoteAnalyzeSupport(UbuntuRemoteRunConfiguration *runConfig,
                                                     Debugger::AnalyzerRunControl *engine, Core::Id runMode)
    : AbstractRemoteRunSupport(runConfig, engine),
      d(new UbuntuRemoteAnalyzeSupportPrivate(engine, runMode))
{
    d->clickPackage = runConfig->clickPackage();
    connect(d->runControl, SIGNAL(starting(const Analyzer::AnalyzerRunControl*)),
            SLOT(handleRemoteSetupRequested()));
    connect(&d->outputParser, SIGNAL(waitingForConnectionOnPort(quint16)),
            SLOT(remoteIsRunning()));
}

UbuntuRemoteAnalyzeSupport::~UbuntuRemoteAnalyzeSupport()
{
    delete d;
}

void UbuntuRemoteAnalyzeSupport::showMessage(const QString &msg, Utils::OutputFormat format)
{
    if (state() != Idle && d->runControl)
        d->runControl->appendMessage(msg, format);
    d->outputParser.processOutput(msg);
}

void UbuntuRemoteAnalyzeSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Idle, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), Utils::NormalMessageFormat);
    AbstractRemoteRunSupport::handleRemoteSetupRequested();
}

void UbuntuRemoteAnalyzeSupport::startExecution()
{
    QTC_ASSERT(state() == ScanningPorts, return);

    // Currently we support only QML profiling
    QTC_ASSERT(d->qmlProfiling, return);

    d->qmlPort = findFreePort();
    if (!d->qmlPort.isValid())
        return;

    setState(Starting);

    UbuntuRemoteClickApplicationRunner *runner = appRunner();
    connect(runner, SIGNAL(launcherStderr(QByteArray)), SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner, SIGNAL(launcherStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(runner, SIGNAL(clickApplicationStarted(quint16)), SLOT(handleRemoteProcessStarted()));
    connect(runner, SIGNAL(finished(bool)), SLOT(handleAppRunnerFinished(bool)));
    connect(runner, SIGNAL(reportError(QString)), SLOT(handleAppRunnerError(QString)));

    runner->setEnv(environment());
    runner->setQmlDebugPort(d->qmlPort.number());

    QTC_ASSERT(device()->type().toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID)),return);
    runner->start(qSharedPointerCast<const UbuntuDevice>(device()),clickPackage(),hook());
}

void UbuntuRemoteAnalyzeSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running)
        showMessage(error, Utils::ErrorMessageFormat);
    else if (state() != Idle)
        handleAdapterSetupFailed(error);
}

void UbuntuRemoteAnalyzeSupport::handleAppRunnerFinished(bool success)
{
    // reset needs to be called first to ensure that the correct state is set.
    reset();
    if (!success)
        showMessage(tr("Failure running remote process."), Utils::NormalMessageFormat);
    d->runControl->notifyRemoteFinished();
}

void UbuntuRemoteAnalyzeSupport::handleProfilingFinished()
{
    setFinished();
}

void UbuntuRemoteAnalyzeSupport::remoteIsRunning()
{
    d->runControl->notifyRemoteSetupDone(d->qmlPort);
}

void UbuntuRemoteAnalyzeSupport::handleRemoteOutput(const QByteArray &output)
{
    QTC_ASSERT(state() == Idle || state() == Running, return);

    showMessage(QString::fromUtf8(output), Utils::StdOutFormat);
}

void UbuntuRemoteAnalyzeSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != ScanningPorts, return);

    if (!d->runControl)
        return;

    showMessage(QString::fromUtf8(output), Utils::StdErrFormat);
}

void UbuntuRemoteAnalyzeSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

void UbuntuRemoteAnalyzeSupport::handleAdapterSetupFailed(const QString &error)
{
    AbstractRemoteRunSupport::handleAdapterSetupFailed(error);
    showMessage(tr("Initial setup failed: %1").arg(error), Utils::NormalMessageFormat);
}

void UbuntuRemoteAnalyzeSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlProfiling, return);
    QTC_ASSERT(state() == Starting, return);

    handleAdapterSetupDone();
}
} // namespace Internal
} // namespace Ubuntu
