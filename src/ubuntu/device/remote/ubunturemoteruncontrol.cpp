/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Canonical Ltd.
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

#include "ubunturemoteruncontrol.h"
#include "ubunturemoterunconfiguration.h"
#include "ubunturemoterunner.h"
#include "ubuntudevice.h"
#include "ubuntuwaitfordevicedialog.h"

#include <remotelinux/abstractremotelinuxrunconfiguration.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>
#include <coreplugin/icore.h>

#include <QString>
#include <QIcon>
#include <QRegularExpression>

using namespace ProjectExplorer;

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteRunControl::UbuntuRemoteRunControlPrivate
{
public:
    bool running;
    UbuntuRemoteClickApplicationRunner runner;
    UbuntuDevice::ConstPtr device;
    QString clickPackage;
    Utils::Environment environment;

    QPointer<UbuntuWaitForDeviceDialog> waitDialog;
    QPointer<UbuntuRemoteRunConfiguration> runConfig;
};

UbuntuRemoteRunControl::UbuntuRemoteRunControl(RunConfiguration *rc)
        : RunControl(rc, ProjectExplorer::Constants::NORMAL_RUN_MODE), d(new UbuntuRemoteRunControlPrivate)
{
    d->running = false;

    d->device = qSharedPointerCast<const UbuntuDevice>(DeviceKitInformation::device(rc->target()->kit()));
    d->runConfig = QPointer<UbuntuRemoteRunConfiguration>(static_cast<UbuntuRemoteRunConfiguration *>(rc));
    d->environment = d->runConfig->environment();
    d->clickPackage = d->runConfig->clickPackage();

    setIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

UbuntuRemoteRunControl::~UbuntuRemoteRunControl()
{
    delete d;
}

void UbuntuRemoteRunControl::start()
{
    if(d->device->deviceState() != ProjectExplorer::IDevice::DeviceReadyToUse) {
        if(d->waitDialog)
            return;

        d->waitDialog = new UbuntuWaitForDeviceDialog(Core::ICore::mainWindow());
        connect(d->waitDialog.data(),SIGNAL(canceled()),this,SLOT(handleWaitDialogCanceled()));
        connect(d->waitDialog.data(),SIGNAL(deviceReady()),this,SLOT(handleDeviceReady()));
        d->waitDialog->show(d->device);

        if(d->device->machineType() == ProjectExplorer::IDevice::Emulator && d->device->deviceState() == ProjectExplorer::IDevice::DeviceDisconnected)
            d->device->helper()->device()->startEmulator();
    } else {
        handleDeviceReady();
    }
}

RunControl::StopResult UbuntuRemoteRunControl::stop()
{
    if(d->waitDialog && !d->running) {
        d->waitDialog->cancel();
        return StoppedSynchronously;
    }
    d->runner.stop();
    return AsynchronousStop;
}

void UbuntuRemoteRunControl::handleErrorMessage(const QString &error)
{
    appendMessage(error, Utils::ErrorMessageFormat);
}

void UbuntuRemoteRunControl::handleRunnerFinished()
{
    setFinished();
}

void UbuntuRemoteRunControl::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
}

void UbuntuRemoteRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
}

void UbuntuRemoteRunControl::handleProgressReport(const QString &progressString)
{
    appendMessage(progressString + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

void UbuntuRemoteRunControl::handleDeviceReady()
{
    d->waitDialog->deleteLater();
    d->running = true;

    if(d->runConfig)
        d->runConfig->setRunning(true);

    emit started();

    d->runner.disconnect(this);

    connect(&d->runner, SIGNAL(reportError(QString)), SLOT(handleErrorMessage(QString)));
    connect(&d->runner, SIGNAL(launcherStderr(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(&d->runner, SIGNAL(launcherStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(&d->runner, SIGNAL(finished(bool)), SLOT(handleRunnerFinished()));

    d->runner.setForceInstall(d->runConfig->forceInstall());
    d->runner.setUninstall(d->runConfig->uninstall());
    d->runner.start(d->device, d->clickPackage,d->runConfig->appId());
}

void UbuntuRemoteRunControl::handleWaitDialogCanceled()
{
    d->waitDialog->deleteLater();
    setFinished();
}

bool UbuntuRemoteRunControl::isRunning() const
{
    return d->running;
}

void UbuntuRemoteRunControl::setFinished()
{
    if(d->runConfig)
        d->runConfig->setRunning(false);
    d->runner.disconnect(this);
    d->running = false;
    emit finished();
}

} // namespace Internal
} // namespace Ubuntu
