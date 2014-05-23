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

#include <remotelinux/abstractremotelinuxrunconfiguration.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>

#include <QString>
#include <QIcon>

using namespace ProjectExplorer;

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteRunControl::UbuntuRemoteRunControlPrivate
{
public:
    bool running;
    ProjectExplorer::DeviceApplicationRunner runner;
    IDevice::ConstPtr device;
    QString remoteExecutable;
    QString clickPackage;
    QStringList arguments;
    Utils::Environment environment;
    QString workingDir;
};

UbuntuRemoteRunControl::UbuntuRemoteRunControl(RunConfiguration *rc)
        : RunControl(rc, ProjectExplorer::NormalRunMode), d(new UbuntuRemoteRunControlPrivate)
{
    d->running = false;
    d->device = DeviceKitInformation::device(rc->target()->kit());
    const UbuntuRemoteRunConfiguration * const lrc = qobject_cast<UbuntuRemoteRunConfiguration *>(rc);
    d->remoteExecutable = lrc->remoteExecutableFilePath();
    d->arguments = lrc->arguments();
    d->environment = lrc->environment();
    d->workingDir = lrc->workingDirectory();
    d->clickPackage = lrc->clickPackage();
}

UbuntuRemoteRunControl::~UbuntuRemoteRunControl()
{
    delete d;
}

void UbuntuRemoteRunControl::start()
{
    d->running = true;
    emit started();
    d->runner.disconnect(this);
    connect(&d->runner, SIGNAL(reportError(QString)), SLOT(handleErrorMessage(QString)));
    connect(&d->runner, SIGNAL(remoteStderr(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(&d->runner, SIGNAL(remoteStdout(QByteArray)), SLOT(handleRemoteOutput(QByteArray)));
    connect(&d->runner, SIGNAL(finished(bool)), SLOT(handleRunnerFinished()));
    connect(&d->runner, SIGNAL(reportProgress(QString)), SLOT(handleProgressReport(QString)));

    QStringList args;

    args << QStringLiteral("/tmp/%1").arg(d->clickPackage);

    Utils::Environment::const_iterator i = d->environment.constBegin();
    for(;i!=d->environment.constEnd();i++) {
        args << QStringLiteral("--env")
             << QStringLiteral("%1:%2").arg(i.key()).arg(i.value());
    }

    d->runner.setWorkingDirectory(d->workingDir);
    d->runner.start(d->device, QStringLiteral("/tmp/qtc_device_applaunch.py").arg(d->clickPackage), args);
}

RunControl::StopResult UbuntuRemoteRunControl::stop()
{
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

bool UbuntuRemoteRunControl::isRunning() const
{
    return d->running;
}

QIcon UbuntuRemoteRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void UbuntuRemoteRunControl::setFinished()
{
    d->runner.disconnect(this);
    d->running = false;
    emit finished();
}

} // namespace Internal
} // namespace Ubuntu
