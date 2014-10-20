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
#include "ubunturemoterunner.h"
#include "ubuntuconstants.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/qtcassert.h>

#include <QPointer>
#include <QTimer>

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
          gdbServerPort(-1), qmlPort(-1)
    {

    }

    const QPointer<Debugger::DebuggerEngine> engine;
    bool qmlDebugging;
    bool cppDebugging;
    QByteArray gdbserverOutput;
    int gdbServerPort;
    int qmlPort;
};

UbuntuRemoteDebugSupport::UbuntuRemoteDebugSupport(UbuntuRemoteRunConfiguration* runConfig,
        Debugger::DebuggerEngine *engine)
    : AbstractRemoteRunSupport(runConfig,engine),
      d(new UbuntuRemoteDebugSupportPrivate(static_cast<UbuntuRemoteRunConfiguration*>(runConfig), engine))
{
    connect(d->engine, SIGNAL(requestRemoteSetup()), this, SLOT(handleRemoteSetupRequested()));
}

UbuntuRemoteDebugSupport::~UbuntuRemoteDebugSupport()
{
    delete d;
}

void UbuntuRemoteDebugSupport::showMessage(const QString &msg, int channel)
{
    if (state() != Idle && d->engine)
        d->engine->showMessage(msg, channel);
}

void UbuntuRemoteDebugSupport::handleRemoteSetupRequested()
{
    showMessage(tr("Checking available ports...") + QLatin1Char('\n'), Debugger::LogStatus);
    AbstractRemoteRunSupport::handleRemoteSetupRequested();
}

void UbuntuRemoteDebugSupport::startExecution()
{
    QTC_ASSERT(state() == ScanningPorts, return);

    setState(Starting);

    if (d->cppDebugging && !assignNextFreePort(&d->gdbServerPort))
        return;
    if (d->qmlDebugging && !assignNextFreePort(&d->qmlPort))
            return;

    d->gdbserverOutput.clear();

    UbuntuRemoteClickApplicationRunner *launcher = appRunner();
    connect(launcher, SIGNAL(launcherStderr(QByteArray)), SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(launcher, SIGNAL(launcherStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));

    if (d->qmlDebugging && !d->cppDebugging)
        connect(launcher, SIGNAL(clickApplicationStarted(quint16)), SLOT(handleRemoteProcessStarted()));

    if(d->cppDebugging)
        launcher->setCppDebugPort(d->gdbServerPort);
    if(d->qmlDebugging)
        launcher->setQmlDebugPort(d->qmlPort);

    launcher->setEnv(environment());

    connect(launcher, SIGNAL(finished(bool)), SLOT(handleAppRunnerFinished(bool)));
    connect(launcher, SIGNAL(reportError(QString)), SLOT(handleAppRunnerError(QString)));

    QTC_ASSERT(device()->type().toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID)),return);
    launcher->start(qSharedPointerCast<const UbuntuDevice>(device()),clickPackage(),hook());
}

void UbuntuRemoteDebugSupport::handleAppRunnerError(const QString &error)
{
    if (state() == Running) {
        showMessage(error, Debugger::AppError);
        if (d->engine)
            d->engine->notifyInferiorIll();
    } else if (state() != Idle) {
        handleAdapterSetupFailed(error);
    }
}

void UbuntuRemoteDebugSupport::handleAppRunnerFinished(bool success)
{
    if (!d->engine || state() == Idle)
        return;

    if (state() == Running) {
        // The QML engine does not realize on its own that the application has finished.
        if (d->qmlDebugging && !d->cppDebugging)
            d->engine->quitDebugger();
        else if (!success)
            d->engine->notifyInferiorIll();

    } else if (state() == Starting){
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
    showMessage(QString::fromUtf8(output), Debugger::AppOutput);
}

void UbuntuRemoteDebugSupport::handleRemoteErrorOutput(const QByteArray &output)
{
    if( state() != ScanningPorts ) {

        if (!d->engine)
            return;

        if (state() == Starting && d->cppDebugging) {
            d->gdbserverOutput += output;

            if (d->gdbserverOutput.contains("Listening on port")) {
                //wait a second for gdb to come up
                handleAdapterSetupDone();
                d->gdbserverOutput.clear();
            }
        }
    }

    showMessage(QString::fromUtf8(output), Debugger::AppError);
}

void UbuntuRemoteDebugSupport::handleProgressReport(const QString &progressOutput)
{
    showMessage(progressOutput + QLatin1Char('\n'), Debugger::LogStatus);
}

void UbuntuRemoteDebugSupport::handleAdapterSetupFailed(const QString &error)
{
    d->engine->notifyEngineRemoteSetupFailed(tr("Initial setup failed: %1").arg(error));
    AbstractRemoteRunSupport::handleAdapterSetupFailed(error);
}

void UbuntuRemoteDebugSupport::handleAdapterSetupDone()
{
    AbstractRemoteRunSupport::handleAdapterSetupDone();
    d->engine->notifyEngineRemoteSetupDone(d->gdbServerPort, d->qmlPort);
}

void UbuntuRemoteDebugSupport::handleRemoteProcessStarted()
{
    QTC_ASSERT(state() == Starting, return);
    QTC_ASSERT(d->qmlDebugging && !d->cppDebugging, return);
    handleAdapterSetupDone();
}

} // namespace Internal
} // namespace Ubuntu
