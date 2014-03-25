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

#include "ubunturuncontrolfactory.h"
#include "ubunturemoterunconfiguration.h"

#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunconfigurationaspect.h>
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

bool UbuntuRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                                ProjectExplorer::RunMode mode) const {

    if(qobject_cast<UbuntuRemoteRunConfiguration*>(runConfiguration)) {
        if (mode != ProjectExplorer::NormalRunMode
                && mode != ProjectExplorer::DebugRunMode
                && mode != ProjectExplorer::DebugRunModeWithBreakOnMain
                && mode != ProjectExplorer::QmlProfilerRunMode) {
            return false;
        }
        return runConfiguration->isEnabled();
    } else {
        if (!qobject_cast<UbuntuProject*>(runConfiguration->target()->project()))
            return false;
        if (mode == ProjectExplorer::NormalRunMode || mode == ProjectExplorer::DebugRunMode)
            return runConfiguration->isEnabled();
    }
    return false;
}

ProjectExplorer::RunControl *UbuntuRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration,
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
            return new RemoteLinux::RemoteLinuxRunControl(rc);
        case ProjectExplorer::DebugRunMode:
        case ProjectExplorer::DebugRunModeWithBreakOnMain: {
            ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(rc->target()->kit());
            if (!dev) {
                *errorMessage = tr("Cannot debug: Kit has no device.");
                return 0;
            }
            if (2 > dev->freePorts().count()) {
                *errorMessage = tr("Cannot debug: Not enough free ports available.");
                return 0;
            }
            Debugger::DebuggerStartParameters params = RemoteLinux::LinuxDeviceDebugSupport::startParameters(rc);
            if (mode == ProjectExplorer::DebugRunModeWithBreakOnMain)
                params.breakOnMain = true;

            params.solibSearchPath.append(rc->soLibSearchPaths());
            qDebug()<<"Solib search path : "<<params.solibSearchPath;

            Debugger::DebuggerRunControl * const runControl
                    = Debugger::DebuggerPlugin::createDebugger(params, rc, errorMessage);
            if (!runControl)
                return 0;
            RemoteLinux::LinuxDeviceDebugSupport * const debugSupport =
                    new RemoteLinux::LinuxDeviceDebugSupport(rc, runControl->engine());
            connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
            return runControl;
        }
        case ProjectExplorer::QmlProfilerRunMode: {
            Analyzer::AnalyzerStartParameters params = RemoteLinux::RemoteLinuxAnalyzeSupport::startParameters(rc, mode);
            Analyzer::AnalyzerRunControl *runControl = Analyzer::AnalyzerManager::createRunControl(params, runConfiguration);
            RemoteLinux::RemoteLinuxAnalyzeSupport * const analyzeSupport =
                    new RemoteLinux::RemoteLinuxAnalyzeSupport(rc, runControl, mode);
            connect(runControl, SIGNAL(finished()), analyzeSupport, SLOT(handleProfilingFinished()));
            return runControl;
        }
        case ProjectExplorer::NoRunMode:
        case ProjectExplorer::CallgrindRunMode:
        case ProjectExplorer::MemcheckRunMode:
            QTC_ASSERT(false, return 0);
        }

        QTC_ASSERT(false, return 0);
        return 0;
    } else {
        QList<ProjectExplorer::RunControl *> runcontrols =
                ProjectExplorer::ProjectExplorerPlugin::instance()->runControls();
        foreach (ProjectExplorer::RunControl *rc, runcontrols) {
            if (UbuntuRunControl *qrc = qobject_cast<UbuntuRunControl *>(rc)) {
                qrc->stop();
            }
        }

        ProjectExplorer::RunControl *runControl = 0;
        if (mode == ProjectExplorer::NormalRunMode)
            runControl = new UbuntuRunControl(runConfiguration, mode, false);
        else if (mode == ProjectExplorer::DebugRunMode)
            runControl = new UbuntuRunControl(runConfiguration, mode, true);

        return runControl;
    }
}

QString UbuntuRunControlFactory::displayName() const {
    return tr("Run on Ubuntu Touch Device");
}
