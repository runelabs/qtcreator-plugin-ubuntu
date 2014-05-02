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

#ifndef UBUNTUPROCESS_H
#define UBUNTUPROCESS_H

#include <QObject>
#include <QProcess>
#include <QFutureInterface>

#include "ubuntu_global.h"

namespace Ubuntu {
namespace Internal {

class UBUNTUSHARED_EXPORT UbuntuProcess : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuProcess(QObject *parent = 0);

    void clear() { m_pendingProcesses.clear(); }

    void append(QStringList cmds) { m_pendingProcesses << cmds; }

    QProcess::ProcessState state() { return m_currentProcess->state(); }

public slots:
    void stop();
    void start(QString taskTitle);

signals:
    void message(const QString&);
    void error (const QString&);
    void stdOut(const QString&);
    void finished(QString,int);
    void finished(const QProcess*,QString,int);
    void started(QString);

protected slots:
    void processStarted();
    void processReadyRead();
    void processFinished(int code);
    void processError(QProcess::ProcessError error);
    void processCmdQueue();

protected:
    void close();
    void kill();

    QList<QStringList> m_pendingProcesses;
    QProcess *m_currentProcess;
    QFutureInterface<void> *m_futureInterface;

    void initializeProgressBar(QString title, int max);
    void setProgressBarFinished();
    void increaseProgress(QString msg);
    void setProgressBarCancelled();
    void setProgressBarStarted();

    bool m_bForceStop;

private:
    Q_DISABLE_COPY(UbuntuProcess)

};


} // Internal
} // Ubuntu

#endif // UBUNTUPROCESS_H
