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

#ifndef UBUNTURUNCONTROL_H
#define UBUNTURUNCONTROL_H

#include <QProcess>
#include <QObject>
#include <QString>

#include "ubuntuproject.h"

#include <utils/environment.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/documentmanager.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qtsupport/qtkitinformation.h>
namespace Ubuntu {
namespace Internal {

class UbuntuRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:

    UbuntuRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode, bool debug = false);
    virtual ~UbuntuRunControl();

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


}
}

#endif // UBUNTURUNCONTROL_H
