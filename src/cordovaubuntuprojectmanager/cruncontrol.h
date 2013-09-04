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
 */

#ifndef CRUNCONTROL_H
#define CRUNCONTROL_H

#include "common.h"

namespace CordovaUbuntuProjectManager {

class CRunControl: public ProjectExplorer::RunControl {
    Q_OBJECT
public:
    CRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode, bool debug = false);
    virtual ~CRunControl();

    void start();
    StopResult stop();
    bool isRunning() const;
    QIcon icon() const;

private slots:
    void processExited(int exitCode);
    void slotBringApplicationToForeground(qint64 pid);
    void slotAppendMessage(const QString &line, Utils::OutputFormat);

private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;

    QString m_executable;
    QString m_commandLineArguments;
    QSharedPointer<QProcess> m_browser;
};

class CRunControlFactory : public ProjectExplorer::IRunControlFactory {
    Q_OBJECT
public:
    explicit CRunControlFactory() = default;
    virtual ~CRunControlFactory() {}

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode) const;
    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                                        ProjectExplorer::RunMode mode, QString *);
    QString displayName() const;
};
}

#endif
