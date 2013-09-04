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

#include "cruncontrol.h"
#include "cproject.h"

namespace CordovaUbuntuProjectManager {

CRunControl::CRunControl(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode, bool debug)
    : RunControl(runConfiguration, mode)
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    if (debug) {
        env.appendOrSet(QLatin1String("QTWEBKIT_INSPECTOR_SERVER"),QLatin1String("9222"));
    }
    m_applicationLauncher.setEnvironment(env);

    m_applicationLauncher.setWorkingDirectory(qobject_cast<CProject*>(runConfiguration->target()->project())->projectDir().absolutePath());

    m_executable = QLatin1String("/usr/bin/cordova-ubuntu-2.8");
    m_commandLineArguments = QLatin1String(".");

    connect(&m_applicationLauncher, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(slotAppendMessage(QString,Utils::OutputFormat)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(slotBringApplicationToForeground(qint64)));
    if (debug) {
        const char *command = CHROMIUM_COMMAND;
        if (QFile(QLatin1String(GOOGLE_CHROME_PATH)).exists())
            command = GOOGLE_CHROME_PATH;
        m_browser = QSharedPointer<QProcess>(new QProcess());
        m_browser->start(QLatin1String(command), QStringList() << QLatin1String("http://localhost:9222/"));
    }
}

CRunControl::~CRunControl() {
    stop();
}

void CRunControl::start() {
    m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui, m_executable,
                                m_commandLineArguments);
    setApplicationProcessHandle(ProjectExplorer::ProcessHandle(m_applicationLauncher.applicationPID()));
    emit started();
    QString msg = tr("Starting %1 %2\n")
            .arg(QDir::toNativeSeparators(m_executable), m_commandLineArguments);
    appendMessage(msg, Utils::NormalMessageFormat);
}

ProjectExplorer::RunControl::StopResult CRunControl::stop() {
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool CRunControl::isRunning() const {
    return m_applicationLauncher.isRunning();
}

QIcon CRunControl::icon() const {
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void CRunControl::slotBringApplicationToForeground(qint64 pid) {
    bringApplicationToForeground(pid);
}

void CRunControl::slotAppendMessage(const QString &line, Utils::OutputFormat format) {
    appendMessage(line, format);
}

void CRunControl::processExited(int exitCode) {
    QString msg = tr("%1 exited with code %2\n")
            .arg(QDir::toNativeSeparators(m_executable)).arg(exitCode);
    appendMessage(msg, exitCode ? Utils::ErrorMessageFormat : Utils::NormalMessageFormat);
    emit finished();
}

bool CRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                                ProjectExplorer::RunMode mode) const {
    if (!qobject_cast<CProject*>(runConfiguration->target()->project()))
        return false;
    if (mode == ProjectExplorer::NormalRunMode || mode == ProjectExplorer::DebugRunMode)
        return true;
    return false;
}

ProjectExplorer::RunControl *CRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration,
                                                        ProjectExplorer::RunMode mode, QString *) {
    QList<ProjectExplorer::RunControl *> runcontrols =
            ProjectExplorer::ProjectExplorerPlugin::instance()->runControls();
    foreach (ProjectExplorer::RunControl *rc, runcontrols) {
        if (CRunControl *qrc = qobject_cast<CRunControl *>(rc)) {
            //TODO: if (qrc->mainQmlFile() == config->mainScript())
            qrc->stop();
        }
    }

    ProjectExplorer::RunControl *runControl = 0;
    if (mode == ProjectExplorer::NormalRunMode)
        runControl = new CRunControl(runConfiguration, mode, false);
    else if (mode == ProjectExplorer::DebugRunMode)
        runControl = new CRunControl(runConfiguration, mode, true);
    return runControl;
}

QString CRunControlFactory::displayName() const {
    return tr("Run");
}

}
