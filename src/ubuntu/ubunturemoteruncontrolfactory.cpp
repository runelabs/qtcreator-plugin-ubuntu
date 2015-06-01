/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#include "ubunturemoteruncontrolfactory.h"
#include "ubunturemoterunconfiguration.h"
#include "ubunturemotedebugsupport.h"
#include "ubunturemoteruncontrol.h"
#include "ubunturemoteanalyzesupport.h"

#include <projectexplorer/kitinformation.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerruncontrol.h>
#include <remotelinux/remotelinuxdebugsupport.h>
#include <remotelinux/remotelinuxruncontrol.h>
#include <remotelinux/remotelinuxanalyzesupport.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/ianalyzertool.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace Ubuntu::Internal;

enum {
    debug = 0
};

bool UbuntuRemoteRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                                ProjectExplorer::RunMode mode) const {

    if(qobject_cast<UbuntuRemoteRunConfiguration*>(runConfiguration)) {
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

ProjectExplorer::RunControl *UbuntuRemoteRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration,
                                                        ProjectExplorer::RunMode mode, QString *errorMessage)
{

    if (qobject_cast<UbuntuRemoteRunConfiguration*>(runConfiguration)) {

        /*
         * Taken from remotelinuxruncontrolfactory.cpp and adapted
         * to work here.
         */
        QTC_ASSERT(canRun(runConfiguration, mode), return 0);

        UbuntuRemoteRunConfiguration *rc = qobject_cast<UbuntuRemoteRunConfiguration *>(runConfiguration);
        QTC_ASSERT(rc, return 0);
        switch (mode) {
        case ProjectExplorer::NormalRunMode:
            return new UbuntuRemoteRunControl(rc);
        case ProjectExplorer::DebugRunMode:
        case ProjectExplorer::DebugRunModeWithBreakOnMain: {
            ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(rc->target()->kit());
            if (!dev) {
                *errorMessage = tr("Cannot debug: Kit has no device.");
                return 0;
            }

            if(debug) qDebug()<<"Free ports on the device: "<<dev->freePorts().count();

            if (2 > dev->freePorts().count()) {
                *errorMessage = tr("Cannot debug: Not enough free ports available.");
                return 0;
            }
            Debugger::DebuggerStartParameters params = RemoteLinux::LinuxDeviceDebugSupport::startParameters(rc);
            if (mode == ProjectExplorer::DebugRunModeWithBreakOnMain)
                params.breakOnMain = true;

            params.solibSearchPath.append(rc->soLibSearchPaths());
            params.runConfiguration = rc;

            //Always leave this empty or the debugger backend tries to execute
            //the binary on the phone instead of attaching and continuing the already
            //running app
            params.remoteExecutable = QString();

            if(debug) qDebug()<<"Solib search path : "<<params.solibSearchPath;

            Debugger::DebuggerRunControl * const runControl
                    = Debugger::createDebuggerRunControl(params, errorMessage);
            if (!runControl)
                return 0;
            UbuntuRemoteDebugSupport * const debugSupport =
                    new UbuntuRemoteDebugSupport(rc, runControl);
            connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
            return runControl;
        }
        case ProjectExplorer::QmlProfilerRunMode: {
            Analyzer::AnalyzerStartParameters params = RemoteLinux::RemoteLinuxAnalyzeSupport::startParameters(rc, mode);
            Analyzer::AnalyzerRunControl *runControl = Analyzer::AnalyzerManager::createRunControl(params, runConfiguration);
            UbuntuRemoteAnalyzeSupport * const analyzeSupport =
                    new UbuntuRemoteAnalyzeSupport(rc, runControl, mode);
            connect(runControl, SIGNAL(finished()), analyzeSupport, SLOT(handleProfilingFinished()));
            return runControl;
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

QString UbuntuRemoteRunControlFactory::displayName() const {
    return tr("Run on Ubuntu Touch Device");
}
