#include "ubuntulocalruncontrolfactory.h"
#include "ubuntulocalrunconfiguration.h"
#include "ubuntulocalscopedebugsupport.h"
#include <ubuntu/ubuntuconstants.h>
#include <ubuntu/clicktoolchain.h>

#include "containerdevice.h"

#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzermanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/abi.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <utils/fileutils.h>
#include <utils/portlist.h>
#include <qmlprofiler/localqmlprofilerrunner.h>
#include <valgrind/callgrindtool.h>
#include <valgrind/memchecktool.h>

#include <remotelinux/remotelinuxruncontrol.h>
#include <remotelinux/remotelinuxdebugsupport.h>
#include <remotelinux/remotelinuxanalyzesupport.h>

#include <QTcpServer>
#include <QSet>

namespace Ubuntu {
namespace Internal {

bool UbuntuLocalRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration, Core::Id mode) const
{
    if(qobject_cast<UbuntuLocalRunConfiguration*>(runConfiguration)) {
        if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
                && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
                && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN
                && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE
                && mode != Valgrind::Internal::CALLGRIND_RUN_MODE
                && mode != Valgrind::MEMCHECK_RUN_MODE
                && mode != Valgrind::MEMCHECK_WITH_GDB_RUN_MODE) {
            return false;
        }

        return runConfiguration->isEnabled();
    }
    return false;
}

ProjectExplorer::RunControl *UbuntuLocalRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    UbuntuLocalRunConfiguration *ubuntuRC = qobject_cast<UbuntuLocalRunConfiguration*>(runConfiguration);
    if (!ubuntuRC)
        return 0;

    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    ProjectExplorer::IDevice::ConstPtr genericDev = ProjectExplorer::DeviceKitInformation::device(runConfiguration->target()->kit());
    if (!genericDev || !genericDev->type().toString().startsWith(QLatin1String(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID))) {
        if(!errorMessage)
            *errorMessage = tr("Wrong or no device assigned to runconfiguration.");
        return 0;
    }

    ProjectExplorer::ToolChain *genericToolchain = ProjectExplorer::ToolChainKitInformation::toolChain(runConfiguration->target()->kit());
    if (!genericToolchain || genericToolchain->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
        if(!errorMessage)
            *errorMessage = tr("Wrong toolchain type in runconfiguration.");
        return 0;
    }

    ContainerDevice::ConstPtr dev = qSharedPointerCast<const ContainerDevice>(genericDev);
    ClickToolChain *tc = static_cast<ClickToolChain *>(genericToolchain);

    if (!ubuntuRC->aboutToStart(errorMessage))
        return 0;

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE) {
        RemoteLinux::RemoteLinuxRunControl *runControl = new RemoteLinux::RemoteLinuxRunControl(ubuntuRC);
        return runControl;
    }

    else if(mode == ProjectExplorer::Constants::DEBUG_RUN_MODE
            || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN) {

        QString rcId = runConfiguration->id().toString();
        bool isScope = rcId.startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_SCOPE_ID));

        if (isScope) {
            Debugger::DebuggerStartParameters params;
            // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
            params.workingDirectory = Utils::FileUtils::normalizePathName(ubuntuRC->workingDirectory());

            QString triplet = tc->gnutriplet();
            if (triplet.isEmpty()) {
                if (errorMessage)
                    *errorMessage = tr("Unsupported host architecture");
                return 0;
            }

            QString scoperunnerPth = QString::fromLatin1("/usr/lib/%1/unity-scopes/scoperunner")
                    .arg(triplet);
            params.executable = QString(UbuntuClickTool::targetBasePath(tc->clickTarget())+scoperunnerPth);
            params.continueAfterAttach = true;
            params.startMode = Debugger::AttachToRemoteServer;
            params.remoteSetupNeeded = true;
            params.connParams.host = dev->sshParameters().host;
            params.environment = ubuntuRC->environment();

            Debugger::DebuggerRunControl *runControl
                    = Debugger::createDebuggerRunControl(params, ubuntuRC, errorMessage, mode);

            //runControl takes ownership of this pointer
            new UbuntuLocalScopeDebugSupport(ubuntuRC, runControl, scoperunnerPth);

            return runControl;
        } else {
            if (ubuntuRC->portsUsedByDebuggers() > dev->freePorts().count()) {
                *errorMessage = tr("Cannot debug: Not enough free ports available.");
                return 0;
            }

            Debugger::DebuggerStartParameters params = RemoteLinux::LinuxDeviceDebugSupport::startParameters(ubuntuRC);
            Debugger::DebuggerRunControl * const runControl = Debugger::createDebuggerRunControl(params, ubuntuRC, errorMessage, mode);
            if (!runControl)
                return 0;

            RemoteLinux::LinuxDeviceDebugSupport * const debugSupport =
                    new RemoteLinux::LinuxDeviceDebugSupport(ubuntuRC, runControl);
            connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
            return runControl;
        }
        return 0;
    }
    else if(mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        Analyzer::AnalyzerStartParameters params = RemoteLinux::RemoteLinuxAnalyzeSupport::startParameters(ubuntuRC, mode);
        Analyzer::AnalyzerRunControl *runControl = Analyzer::AnalyzerManager::createRunControl(params, ubuntuRC);
        (void) new RemoteLinux::RemoteLinuxAnalyzeSupport(ubuntuRC, runControl, mode);
        return runControl;
    }
    QTC_ASSERT(false, return 0);

}

QString UbuntuLocalRunControlFactory::displayName() const
{
    return tr("Run Ubuntu project locally");
}

} //namespace Internal
} //namespace Ubuntu
