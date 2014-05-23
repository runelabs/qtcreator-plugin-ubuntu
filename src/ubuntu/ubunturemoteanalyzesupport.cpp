#include "ubunturemoteanalyzesupport.h"
#include "ubunturemoterunconfiguration.h"

#include <remotelinux/remotelinuxrunconfiguration.h>
#include <analyzerbase/analyzerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>

#include <utils/qtcassert.h>
#include <qmldebug/qmloutputparser.h>

#include <QPointer>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteAnalyzeSupportPrivate
{
public:
    UbuntuRemoteAnalyzeSupportPrivate(Analyzer::AnalyzerRunControl *rc, ProjectExplorer::RunMode runMode)
        : runControl(rc),
          qmlProfiling(runMode == ProjectExplorer::QmlProfilerRunMode),
          qmlPort(-1)
    {
    }

    QString clickPackage;
    const QPointer<Analyzer::AnalyzerRunControl> runControl;
    bool qmlProfiling;
    int qmlPort;

    QmlDebug::QmlOutputParser outputParser;
};

UbuntuRemoteAnalyzeSupport::UbuntuRemoteAnalyzeSupport(UbuntuRemoteRunConfiguration *runConfig,
                                                     Analyzer::AnalyzerRunControl *engine, ProjectExplorer::RunMode runMode)
    : AbstractRemoteLinuxRunSupport(runConfig, engine),
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
    if (state() != Inactive && d->runControl)
        d->runControl->logApplicationMessage(msg, format);
    d->outputParser.processOutput(msg);
}

void UbuntuRemoteAnalyzeSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), Utils::NormalMessageFormat);
    AbstractRemoteLinuxRunSupport::handleRemoteSetupRequested();
}

void UbuntuRemoteAnalyzeSupport::startExecution()
{
    QTC_ASSERT(state() == GatheringPorts, return);

    // Currently we support only QML profiling
    QTC_ASSERT(d->qmlProfiling, return);

    if (!setPort(d->qmlPort))
        return;

    setState(StartingRunner);

    ProjectExplorer::DeviceApplicationRunner *runner = appRunner();
    connect(runner, SIGNAL(remoteStderr(QByteArray)), SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(runner, SIGNAL(remoteProcessStarted()), SLOT(handleRemoteProcessStarted()));
    connect(runner, SIGNAL(finished(bool)), SLOT(handleAppRunnerFinished(bool)));
    connect(runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));
    connect(runner, SIGNAL(reportError(QString)), SLOT(handleAppRunnerError(QString)));

    QStringList args = arguments();
    QString command = QStringLiteral("/tmp/qtc_device_applaunch.py");
    args << QStringLiteral("/tmp/%1").arg(d->clickPackage);

    Utils::Environment env = environment();
    Utils::Environment::const_iterator i = env.constBegin();
    for(;i!=env.constEnd();i++) {
        args << QStringLiteral("--env")
             << QStringLiteral("%1:%2").arg(i.key()).arg(i.value());
    }

    args.append(QStringLiteral("--qmldebug=port:%1,block").arg(d->qmlPort));

    runner->setWorkingDirectory(workingDirectory());
    runner->start(device(), command, args);
}

void UbuntuRemoteAnalyzeSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running)
        showMessage(error, Utils::ErrorMessageFormat);
    else if (state() != Inactive)
        handleAdapterSetupFailed(error);
}

void UbuntuRemoteAnalyzeSupport::handleAppRunnerFinished(bool success)
{
    // reset needs to be called first to ensure that the correct state is set.
    reset();
    if (!success)
        showMessage(tr("Failure running remote process."), Utils::NormalMessageFormat);
    d->runControl->notifyRemoteFinished(success);
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
    QTC_ASSERT(state() == Inactive || state() == Running, return);

    showMessage(QString::fromUtf8(output), Utils::StdOutFormat);
}

void UbuntuRemoteAnalyzeSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != GatheringPorts, return);

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
    AbstractRemoteLinuxRunSupport::handleAdapterSetupFailed(error);
    showMessage(tr("Initial setup failed: %1").arg(error), Utils::NormalMessageFormat);
}

void UbuntuRemoteAnalyzeSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlProfiling, return);
    QTC_ASSERT(state() == StartingRunner, return);

    handleAdapterSetupDone();
}
} // namespace Internal
} // namespace Ubuntu
