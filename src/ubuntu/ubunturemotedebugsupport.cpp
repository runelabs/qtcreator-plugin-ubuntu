/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "ubunturemotedebugsupport.h"
#include "ubunturemoterunconfiguration.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>

#include <utils/qtcassert.h>

#include <QPointer>
#include <debugger/debuggerrunconfigurationaspect.h>

namespace Ubuntu {
namespace Internal {


class UbuntuRemoteDebugSupportPrivate
{
public:
    UbuntuRemoteDebugSupportPrivate(const UbuntuRemoteRunConfiguration *runConfig,
            Debugger::DebuggerEngine *engine)
        : engine(engine),
          qmlDebugging(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useQmlDebugger()),
          cppDebugging(runConfig->extraAspect<Debugger::DebuggerRunConfigurationAspect>()->useCppDebugger()),
          gdbServerPort(-1), qmlPort(-1),
          clickPackage(runConfig->clickPackage())
    {

    }

    const QPointer<Debugger::DebuggerEngine> engine;
    bool qmlDebugging;
    bool cppDebugging;
    QByteArray gdbserverOutput;
    int gdbServerPort;
    int qmlPort;

    QString clickPackage;
    int procPid;
    QString output;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr deviceSignalOp;
};

UbuntuRemoteDebugSupport::UbuntuRemoteDebugSupport(UbuntuRemoteRunConfiguration* runConfig,
        Debugger::DebuggerEngine *engine)
    : AbstractRemoteLinuxRunSupport(runConfig, engine),
      d(new UbuntuRemoteDebugSupportPrivate(static_cast<UbuntuRemoteRunConfiguration*>(runConfig), engine))
{
    connect(d->engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleRemoteSetupRequested()));
    d->deviceSignalOp = device()->signalOperation();
}

UbuntuRemoteDebugSupport::~UbuntuRemoteDebugSupport()
{
    delete d;
}

void UbuntuRemoteDebugSupport::showMessage(const QString &msg, int channel)
{
    if (state() != Inactive && d->engine)
        d->engine->showMessage(msg, channel);
}

void UbuntuRemoteDebugSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(state() == Inactive, return);

    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), Debugger::LogStatus);
    AbstractRemoteLinuxRunSupport::handleRemoteSetupRequested();
}

void UbuntuRemoteDebugSupport::startExecution()
{
    QTC_ASSERT(state() == GatheringPorts, return);

    if (d->cppDebugging && !setPort(d->gdbServerPort))
        return;
    if (d->qmlDebugging && !setPort(d->qmlPort))
            return;

    setState(StartingRunner);
    d->gdbserverOutput.clear();

    ProjectExplorer::DeviceApplicationRunner *runner = appRunner();
    connect(runner, SIGNAL(remoteStderr(QByteArray)), SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    if (d->qmlDebugging && !d->cppDebugging)
        connect(runner, SIGNAL(remoteProcessStarted()), SLOT(handleRemoteProcessStarted()));

    QStringList args;
    QString command = QStringLiteral("/tmp/qtc_device_applaunch.py");
    args << QStringLiteral("/tmp/%1").arg(d->clickPackage);

    Utils::Environment env = environment();
    Utils::Environment::const_iterator i = env.constBegin();
    for(;i!=env.constEnd();i++) {
        args << QStringLiteral("--env")
             << QStringLiteral("%1:%2").arg(i.key()).arg(i.value());
    }

    if (d->qmlDebugging)
        args.append(QStringLiteral("--qmldebug=port:%1,block").arg(d->qmlPort));
    if (d->cppDebugging)
        args.append(QStringLiteral("--cppdebug=%1").arg(d->gdbServerPort));

    connect(runner, SIGNAL(finished(bool)), SLOT(handleAppRunnerFinished(bool)));
    connect(runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));
    connect(runner, SIGNAL(reportError(QString)), SLOT(handleAppRunnerError(QString)));
    runner->setEnvironment(environment());
    runner->setWorkingDirectory(workingDirectory());
    runner->start(device(), command, args);
}

void UbuntuRemoteDebugSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, Debugger::AppError);
        if (d->engine)
            d->engine->notifyInferiorIll();
    } else if (state() != Inactive) {
        handleAdapterSetupFailed(error);
    }
}

void UbuntuRemoteDebugSupport::handleAppRunnerFinished(bool success)
{
    if (!d->engine || state() == Inactive)
        return;

    if (state() == Running) {
        // The QML engine does not realize on its own that the application has finished.
        if (d->qmlDebugging && !d->cppDebugging)
            d->engine->quitDebugger();
        else if (!success)
            d->engine->notifyInferiorIll();

    } else if (state() == StartingRunner){
        d->engine->notifyEngineRemoteSetupFailed(tr("Debugging failed."));
    }
    reset();
}

void UbuntuRemoteDebugSupport::handleDebuggingFinished()
{
    setFinished();
    reset();
}

void UbuntuRemoteDebugSupport::handleRemoteOutput(const QByteArray &output)
{
    //QTC_ASSERT(state() == Inactive || state() == Running, return);

    if( state()!= GatheringPorts ) {

        if (!d->engine)
            return;

        if (state() == StartingRunner && d->cppDebugging) {
            d->gdbserverOutput += output;

            //if (d->gdbserverOutput.contains("Listening on port")) {
            if (d->gdbserverOutput.contains("Application started")) {
                handleAdapterSetupDone();
                d->gdbserverOutput.clear();
            }
        }
    }

    if( state() == Inactive || state() == Running )
        showMessage(QString::fromUtf8(output), Debugger::AppOutput);

}

void UbuntuRemoteDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    QTC_ASSERT(state() != GatheringPorts, return);


    showMessage(QString::fromUtf8(output), Debugger::AppError);
}

void UbuntuRemoteDebugSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), Debugger::LogStatus);
}

void UbuntuRemoteDebugSupport::handleAdapterSetupFailed(const QString &error)
{
    AbstractRemoteLinuxRunSupport::handleAdapterSetupFailed(error);
    d->engine->notifyEngineRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
}

void UbuntuRemoteDebugSupport::handleAdapterSetupDone()
{
    AbstractRemoteLinuxRunSupport::handleAdapterSetupDone();
    d->engine->notifyEngineRemoteSetupDone(d->gdbServerPort, d->qmlPort);
}

void UbuntuRemoteDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->qmlDebugging && !d->cppDebugging, return);
    QTC_ASSERT(state() == StartingRunner, return);

    handleAdapterSetupDone();
}

} // namespace Internal
} // namespace Ubuntu
