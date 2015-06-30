#include "ubuntulocalruncontrolfactory.h"
#include "ubuntulocalrunconfiguration.h"
#include "ubuntulocalscopedebugsupport.h"
#include "ubuntuconstants.h"
#include "clicktoolchain.h"

#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzermanager.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/abi.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <utils/fileutils.h>
#include <qmlprofiler/localqmlprofilerrunner.h>

#include <QTcpServer>
#include <QSet>

namespace Ubuntu {
namespace Internal {

bool UbuntuLocalRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode) const
{
    if(qobject_cast<UbuntuLocalRunConfiguration*>(runConfiguration)) {
        switch (mode) {
            case ProjectExplorer::NormalRunMode:
            case ProjectExplorer::DebugRunMode:
            case ProjectExplorer::DebugRunModeWithBreakOnMain:
            case ProjectExplorer::QmlProfilerRunMode:
            case ProjectExplorer::CallgrindRunMode:
            case ProjectExplorer::MemcheckRunMode:
            case ProjectExplorer::MemcheckWithGdbRunMode:
                return runConfiguration->isEnabled();
            default:
                return false;
        }
    }
    return false;
}

ProjectExplorer::RunControl *UbuntuLocalRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode, QString *errorMessage)
{
    UbuntuLocalRunConfiguration *ubuntuRC = qobject_cast<UbuntuLocalRunConfiguration*>(runConfiguration);
    if (ubuntuRC) {
        QTC_ASSERT(canRun(runConfiguration, mode), return 0);

        if (!ubuntuRC->aboutToStart(errorMessage))
            return 0;

        switch (mode) {
        case ProjectExplorer::NormalRunMode: {
            ProjectExplorer::LocalApplicationRunControl *runControl =
                    new ProjectExplorer::LocalApplicationRunControl(ubuntuRC, mode);
            runControl->setCommand(ubuntuRC->executable(), ubuntuRC->commandLineArguments());
            runControl->setApplicationLauncherMode(ubuntuRC->runMode());
            runControl->setWorkingDirectory(ubuntuRC->workingDirectory());
            return runControl;
        }
        case ProjectExplorer::DebugRunMode:
        case ProjectExplorer::DebugRunModeWithBreakOnMain: {

            QString rcId = runConfiguration->id().toString();
            bool isScope = rcId.startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_SCOPE_ID));

            Debugger::DebuggerStartParameters params;
            // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
            params.workingDirectory = Utils::FileUtils::normalizePathName(ubuntuRC->workingDirectory());
            params.useTerminal = ubuntuRC->runMode() == ProjectExplorer::ApplicationLauncher::Console;

            params.executable = ubuntuRC->executable();
            if (params.executable.isEmpty())
                return 0;

            params.processArgs = ubuntuRC->commandLineArguments();

            if (isScope) {
                ProjectExplorer::Abi hostAbi = ProjectExplorer::Abi::hostAbi();
                if (hostAbi.os() == ProjectExplorer::Abi::LinuxOS) {
                    QString triplet = ClickToolChain::gnutriplet(hostAbi);
                    if (triplet.isEmpty()) {
                        if (errorMessage)
                            *errorMessage = tr("Unsupported host architecture");
                        return 0;
                    }

                    params.executable = QString::fromLatin1("/usr/lib/%1/unity-scopes/scoperunner")
                            .arg(triplet);
                } else {
                    if (errorMessage)
                        *errorMessage = tr("Running scopes is not implemented on this OS.");
                    return 0;
                }

                params.processArgs.clear();
                params.continueAfterAttach = true;
                params.startMode = Debugger::AttachToRemoteServer;
                params.remoteSetupNeeded = true;
                params.connParams.host = QStringLiteral("127.0.0.1");
            }

            Debugger::DebuggerRunControl *runControl
                    = Debugger::createDebuggerRunControl(params, ubuntuRC, errorMessage, mode);

            if (isScope) {
                //runControl takes ownership of this pointer
                new UbuntuLocalScopeDebugSupport(ubuntuRC, runControl, params.executable);
            }

            return runControl;
        }
        case ProjectExplorer::QmlProfilerRunMode: {

            ProjectExplorer::EnvironmentAspect *environment =
                    ubuntuRC->extraAspect<ProjectExplorer::EnvironmentAspect>();

            Analyzer::AnalyzerStartParameters sp;
            sp.runMode = mode;
            sp.workingDirectory = ubuntuRC->workingDirectory();
            sp.debuggee = ubuntuRC->executable();
            sp.debuggeeArgs = ubuntuRC->commandLineArguments();
            sp.displayName = ubuntuRC->displayName();
            if (environment)
                sp.environment = environment->environment();

            sp.analyzerPort = QmlProfiler::LocalQmlProfilerRunner::findFreePort(sp.analyzerHost);
            if(sp.analyzerPort == 0) {
                *errorMessage = tr("Cannot open port on host for QML profiling.");
                return 0;
            }

            return QmlProfiler::LocalQmlProfilerRunner::createLocalRunControl(runConfiguration, sp,errorMessage);
        }
        case ProjectExplorer::CallgrindRunMode:
        case ProjectExplorer::MemcheckRunMode:
        case ProjectExplorer::MemcheckWithGdbRunMode: {
            Analyzer::AnalyzerStartParameters sp;
            sp.displayName = ubuntuRC->displayName();
            sp.runMode = mode;
            ProjectExplorer::EnvironmentAspect *aspect
                    = ubuntuRC->extraAspect<ProjectExplorer::EnvironmentAspect>();
            if (aspect)
                sp.environment = aspect->environment();
            sp.workingDirectory = ubuntuRC->workingDirectory();
            sp.debuggee = ubuntuRC->executable();
            sp.debuggeeArgs = ubuntuRC->commandLineArguments();
            const ProjectExplorer::IDevice::ConstPtr device =
                    ProjectExplorer::DeviceKitInformation::device(ubuntuRC->target()->kit());
            QTC_ASSERT(device, return 0);
            QTC_ASSERT(device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE, return 0);
            QTcpServer server;
            if (!server.listen(QHostAddress::LocalHost) && !server.listen(QHostAddress::LocalHostIPv6)) {
                *errorMessage = tr("Cannot open port on host for profiling.");
                return 0;
            }
            sp.connParams.host = server.serverAddress().toString();
            sp.connParams.port = server.serverPort();
            sp.localRunMode = static_cast<ProjectExplorer::ApplicationLauncher::Mode>(ubuntuRC->runMode());


            return Analyzer::AnalyzerManager::createRunControl(sp, runConfiguration);
        }
        case ProjectExplorer::NoRunMode:
        case ProjectExplorer::ClangStaticAnalyzerMode:
        case ProjectExplorer::PerfProfilerRunMode:
            QTC_ASSERT(false, return 0);
        }
        QTC_ASSERT(false, return 0);
    }
    return 0;

}

QString UbuntuLocalRunControlFactory::displayName() const
{
    return tr("Run Ubuntu project locally");
}

} //namespace Internal
} //namespace Ubuntu
