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

#include "ubunturuncontrol.h"

using namespace Ubuntu;
using namespace Ubuntu::Internal;


UbuntuRunControl::UbuntuRunControl(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode, bool debug)
    : RunControl(runConfiguration, mode)
{
    Utils::Environment env = Utils::Environment::systemEnvironment();

    m_applicationLauncher.setEnvironment(env);
    UbuntuProject* ubuntuProject = qobject_cast<UbuntuProject*>(runConfiguration->target()->project());

    m_applicationLauncher.setWorkingDirectory(ubuntuProject->projectDir().absolutePath());

    if (ubuntuProject->mainFile().compare(QString::fromLatin1("www/index.html"), Qt::CaseInsensitive) == 0) {
        //TODO move into abstracted location
        m_executable = QString::fromLatin1(Ubuntu::Constants::UBUNTUHTML_PROJECT_LAUNCHER_EXE);
        m_commandLineArguments = QString(QLatin1String("--www=%0/www --inspector")).arg(ubuntuProject->projectDirectory());
    }
    else {
        m_executable = QtSupport::QtKitInformation::qtVersion(runConfiguration->target()->kit())->qmlsceneCommand();
        m_commandLineArguments = QString(QLatin1String("%0.qml")).arg(ubuntuProject->displayName());
    }

    connect(&m_applicationLauncher, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(slotAppendMessage(QString,Utils::OutputFormat)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(slotBringApplicationToForeground(qint64)));
}

UbuntuRunControl::~UbuntuRunControl() {
    stop();
}

void UbuntuRunControl::start() {
    qDebug() << __PRETTY_FUNCTION__;
    m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui, m_executable,
                                m_commandLineArguments);
    setApplicationProcessHandle(ProjectExplorer::ProcessHandle(m_applicationLauncher.applicationPID()));
    emit started();
    QString msg = tr("Starting %1 %2\n")
            .arg(QDir::toNativeSeparators(m_executable), m_commandLineArguments);
    appendMessage(msg, Utils::NormalMessageFormat);
}

ProjectExplorer::RunControl::StopResult UbuntuRunControl::stop() {
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool UbuntuRunControl::isRunning() const {
    return m_applicationLauncher.isRunning();
}

QIcon UbuntuRunControl::icon() const {
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void UbuntuRunControl::slotBringApplicationToForeground(qint64 pid) {
    bringApplicationToForeground(pid);
}

void UbuntuRunControl::slotAppendMessage(const QString &line, Utils::OutputFormat format) {
    appendMessage(line, format);
}

void UbuntuRunControl::processExited(int exitCode) {
    QString msg = tr("%1 exited with code %2\n")
            .arg(QDir::toNativeSeparators(m_executable)).arg(exitCode);
    appendMessage(msg, exitCode ? Utils::ErrorMessageFormat : Utils::NormalMessageFormat);
    emit finished();
}




