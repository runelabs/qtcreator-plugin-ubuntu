#include "ubuntulocalruncontrolfactory.h"
#include "ubuntulocalrunconfiguration.h"

#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <projectexplorer/localapplicationruncontrol.h>
#include <projectexplorer/applicationlauncher.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <utils/fileutils.h>
#include <qmlprofiler/localqmlprofilerrunner.h>

namespace Ubuntu {
namespace Internal {

bool UbuntuLocalRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode) const
{
    if(qobject_cast<UbuntuLocalRunConfiguration*>(runConfiguration)) {
        if (mode != ProjectExplorer::NormalRunMode
                && mode != ProjectExplorer::DebugRunMode
                && mode != ProjectExplorer::DebugRunModeWithBreakOnMain
                && mode != ProjectExplorer::QmlProfilerRunMode) {
            return false;
        }
        return runConfiguration->isEnabled();
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

            Debugger::DebuggerStartParameters params;
            if (!Debugger::fillParametersFromRunConfiguration(&params,runConfiguration,errorMessage))
                return 0;

            // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
            params.workingDirectory = Utils::FileUtils::normalizePathName(ubuntuRC->workingDirectory());

            params.executable = ubuntuRC->executable();
            if (params.executable.isEmpty())
                return 0;

            params.processArgs = ubuntuRC->commandLineArguments();
            params.useTerminal = ubuntuRC->runMode() == ProjectExplorer::ApplicationLauncher::Console;

            return Debugger::createDebuggerRunControl(params, errorMessage);
        }
        case ProjectExplorer::QmlProfilerRunMode: {

            ProjectExplorer::EnvironmentAspect *environment =
                    ubuntuRC->extraAspect<ProjectExplorer::EnvironmentAspect>();

            Analyzer::AnalyzerStartParameters sp;
            sp.startMode = Analyzer::StartLocal;
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
        case ProjectExplorer::NoRunMode:
        case ProjectExplorer::CallgrindRunMode:
        case ProjectExplorer::MemcheckRunMode:
        case ProjectExplorer::MemcheckWithGdbRunMode:
        case ProjectExplorer::ClangStaticAnalyzerMode:
        case ProjectExplorer::PerfProfilerRunMode:
            QTC_ASSERT(false, return 0);
        }

        QTC_ASSERT(false, return 0);
        return 0;
    }
    return 0;

}

QString UbuntuLocalRunControlFactory::displayName() const
{
    return tr("Run Ubuntu project locally");
}

} //namespace Internal
} //namespace Ubuntu
