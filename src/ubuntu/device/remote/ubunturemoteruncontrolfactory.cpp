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
#include <qmldebug/qmldebugcommandlinearguments.h>
#include <remotelinux/remotelinuxdebugsupport.h>
#include <remotelinux/remotelinuxruncontrol.h>
#include <remotelinux/remotelinuxanalyzesupport.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerruncontrol.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace Ubuntu::Internal;

enum {
    debug = 0
};

bool UbuntuRemoteRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                                Core::Id mode) const {

    if(qobject_cast<UbuntuRemoteRunConfiguration*>(runConfiguration)) {
        if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
                && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
                && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN
                && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
            return false;
        }
        return runConfiguration->isEnabled();
    }
    return false;
}

ProjectExplorer::RunControl *UbuntuRemoteRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration,
                                                        Core::Id mode, QString *errorMessage)
{

    if (qobject_cast<UbuntuRemoteRunConfiguration*>(runConfiguration)) {

        /*
         * Taken from remotelinuxruncontrolfactory.cpp and adapted
         * to work here.
         */
        QTC_ASSERT(canRun(runConfiguration, mode), return 0);

        UbuntuRemoteRunConfiguration *rc = static_cast<UbuntuRemoteRunConfiguration *>(runConfiguration);
        if (!rc->aboutToStart(errorMessage))
            return 0;

        QTC_ASSERT(rc, return 0);        
        const auto rcRunnable = runConfiguration->runnable();
        QTC_ASSERT(rcRunnable.is<ProjectExplorer::StandardRunnable>(), return 0);
        const auto stdRunnable = rcRunnable.as<ProjectExplorer::StandardRunnable>();


        if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE) {
            return new UbuntuRemoteRunControl(rc);

        } else if ( mode == ProjectExplorer::Constants::DEBUG_RUN_MODE
                    || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN ) {
            ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(rc->target()->kit());
            if (!dev) {
                *errorMessage = tr("Cannot debug: Kit has no device.");
                return 0;
            }

            if(debug) qDebug()<<"Free ports on the device: "<<dev->freePorts().count();

            if (rc->localExecutableFilePath().isEmpty()) {
                if (errorMessage) *errorMessage = tr("The current project has no support for running in a debugger.");
                return 0;
            }

            auto aspect = rc->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
            if (aspect->portsUsedByDebugger() > dev->freePorts().count()) {
                *errorMessage = tr("Cannot debug: Not enough free ports available.");
                return 0;
            }

            /*
             * Taken from remotelinuxruncontrolfactory.cpp and adapted
             * to work here.
             */
            Debugger::DebuggerStartParameters params;
            params.startMode = Debugger::AttachToRemoteServer;
            params.closeMode = Debugger::KillAndExitMonitorAtClose;
            params.remoteSetupNeeded = true;
            params.useContinueInsteadOfRun = true;

            if (aspect->useQmlDebugger()) {
                params.qmlServer.host = dev->sshParameters().host;
                params.qmlServer.port = Utils::Port(); // port is selected later on
            }
            if (aspect->useCppDebugger()) {
                aspect->setUseMultiProcess(true);
#if 1
                params.inferior.executable = stdRunnable.executable;
                params.inferior.commandLineArguments = stdRunnable.commandLineArguments;
                if (aspect->useQmlDebugger()) {
                    params.inferior.commandLineArguments.prepend(QLatin1Char(' '));
                    params.inferior.commandLineArguments.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices));
                }
#endif
                params.remoteChannel = dev->sshParameters().host + QLatin1String(":-1");
                params.symbolFile = rc->localExecutableFilePath();
            }

            params.solibSearchPath.append(rc->soLibSearchPaths());

            Debugger::DebuggerRunControl * const runControl = Debugger::createDebuggerRunControl(params, rc, errorMessage, mode);
            if (!runControl)
                return 0;

            UbuntuRemoteDebugSupport * const debugSupport =
                    new UbuntuRemoteDebugSupport(rc, runControl);
            connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
            return runControl;

        } else if ( mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE ) {
            /*
             * Taken from remotelinuxruncontrolfactory.cpp and adapted
             * to work here.
             */
            auto runControl = Debugger::createAnalyzerRunControl(rc, mode);
            Debugger::AnalyzerConnection connection;
            connection.connParams =
                ProjectExplorer::DeviceKitInformation::device(rc->target()->kit())->sshParameters();
            connection.analyzerHost = connection.connParams.host;
            runControl->setConnection(connection);
            UbuntuRemoteAnalyzeSupport * const analyzeSupport =
                    new UbuntuRemoteAnalyzeSupport(rc, runControl, mode);
            connect(runControl, SIGNAL(finished()), analyzeSupport, SLOT(handleProfilingFinished()));
            return runControl;
        }

        QTC_ASSERT(false, return 0);
    }
    return 0;
}

QString UbuntuRemoteRunControlFactory::displayName() const {
    return tr("Run on Ubuntu Touch Device");
}
