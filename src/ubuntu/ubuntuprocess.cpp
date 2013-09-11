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

#include "ubuntuprocess.h"

#include <QVariant>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/icore.h>
#include "ubuntuconstants.h"

using namespace Ubuntu;
using namespace Ubuntu::Internal;

UbuntuProcess::UbuntuProcess(QObject *parent) :
    QObject(parent),
    m_futureInterface(0)
{
    connect(&m_currentProcess,SIGNAL(readyReadStandardError()),this,SLOT(processReadyRead()));
    connect(&m_currentProcess,SIGNAL(started()),this,SLOT(processStarted()));
    connect(&m_currentProcess,SIGNAL(finished(int)),this,SLOT(processFinished(int)));
    connect(&m_currentProcess,SIGNAL(readyRead()),this,SLOT(processReadyRead()));
    connect(&m_currentProcess,SIGNAL(error(QProcess::ProcessError)),this,SLOT(processError(QProcess::ProcessError)));
}

void UbuntuProcess::initializeProgressBar(QString title, int max) {
   if (m_futureInterface) {
        m_futureInterface->reportCanceled();
        delete m_futureInterface;
        m_futureInterface = 0;
    }

    m_futureInterface = new QFutureInterface<void>();

    m_futureInterface->setProgressRange(0,max);

    Core::FutureProgress* futureProgress = Core::ICore::progressManager()->addTask(m_futureInterface->future(),title,QLatin1String(Constants::TASK_DEVICE_SCRIPT));

    connect(futureProgress,SIGNAL(clicked()),this,SLOT(stop()));
}

void UbuntuProcess::setProgressBarStarted() {
    m_futureInterface->reportStarted();
}

void UbuntuProcess::setProgressBarFinished() {
    m_futureInterface->reportFinished();
}

void UbuntuProcess::increaseProgress(QString msg) {
    int currentValue = m_futureInterface->progressValue();
    m_futureInterface->setProgressValueAndText(currentValue++,msg);
}

void UbuntuProcess::setProgressBarCancelled() {
    m_futureInterface->reportCanceled();
    m_futureInterface->reportFinished();
}

void UbuntuProcess::close() {
    m_currentProcess.close();
    m_currentProcess.waitForFinished();
}

void UbuntuProcess::stop() {
    kill();
}

void UbuntuProcess::processStarted() {
    increaseProgress(m_currentProcess.program());
    emit started(m_currentProcess.program());
}

void UbuntuProcess::kill() {
    this->clear();
    m_currentProcess.kill();
    m_currentProcess.waitForFinished();
}

void UbuntuProcess::processError(QProcess::ProcessError err) {
    Q_UNUSED(err);
    if (m_currentProcess.exitCode() == 0) { return; }
    emit error(QString(QLatin1String("ERROR: (%0) %1")).arg(m_currentProcess.program()).arg(m_currentProcess.errorString()));

    // lets clear the remaining actions
    m_pendingProcesses.clear();

    setProgressBarCancelled();
}

void UbuntuProcess::start(QString taskTitle) {
    initializeProgressBar(taskTitle,m_pendingProcesses.length());
    processCmdQueue();
}

void UbuntuProcess::processFinished(int code) {
    if (code != 0) {
        emit error(QString(QLatin1String(m_currentProcess.readAllStandardError())));
        m_pendingProcesses.clear();
        setProgressBarCancelled();
        return;
    }
    QString errorMsg = QString::fromLatin1(m_currentProcess.readAllStandardError());
    if (errorMsg.trimmed().length()>0) emit error(errorMsg);
    QString msg = QString::fromLatin1(m_currentProcess.readAllStandardOutput());
    if (msg.trimmed().length()>0) emit message(msg);

    emit finished(m_currentProcess.program(), code);
    processCmdQueue();
}

void UbuntuProcess::processReadyRead() {
    QString stderr = QString::fromLatin1(m_currentProcess.readAllStandardError());
    QString stdout = QString::fromLatin1(m_currentProcess.readAllStandardOutput());
    if (!stderr.isEmpty()) {
        emit message(stderr);
    }
    if (!stdout.isEmpty()) {
        emit message(stdout);
    }
}

void UbuntuProcess::processCmdQueue() {
    if (m_pendingProcesses.length() == 0) {
        setProgressBarFinished();
        return;
    }

    QStringList cmdList = m_pendingProcesses.takeFirst();

    QString cmd = cmdList.takeFirst();

    QStringList args;
    QString workingDirectory;
    if (cmdList.length()>0) {
        workingDirectory = cmdList.takeLast();
    }

    if (cmdList.length()>0) {
        args << cmdList;
    }

    if (!workingDirectory.isEmpty()) {
        m_currentProcess.setWorkingDirectory(workingDirectory);
    }

    QString msg(QString(QLatin1String("%0 %1")).arg(cmd).arg(args.join(QLatin1String(" "))));
    //emit message(msg);
    m_currentProcess.setProperty("command",cmd);

    if (args.length()>0) {
        m_currentProcess.start(cmd,args);
    } else {
        m_currentProcess.start(cmd);
    }
}
