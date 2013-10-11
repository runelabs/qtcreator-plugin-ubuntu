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

#include "plugin.h"
#include "constants.h"
#include "cprojectmanager.h"
#include "cruncontrol.h"
#include "crunconfiguration.h"

#include <ubuntu/ubuntuconstants.h>

namespace CordovaUbuntuProjectManager {

const char FOLDERNAME[] = "%FOLDERNAME%";
const char *RUN_ON_DEVICE[] = {"tar -cjf ../%FOLDERNAME%.tar.bz2 ../%FOLDERNAME%",
                               "scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -P2222 ../%FOLDERNAME%.tar.bz2 phablet@127.0.0.1:~",
                               "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p2222 phablet@127.0.0.1 tar -xvf %FOLDERNAME%.tar.bz2",
                               "bash -ic \"echo '[Desktop Entry]\nName=PhoneGap Demo\nType=Application\nExec=cordova-ubuntu-2.8\nIcon=\n' > /tmp/%FOLDERNAME%.desktop\"",
                               "scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -P2222 /tmp/%FOLDERNAME%.desktop phablet@127.0.0.1:~/%FOLDERNAME%",
                               "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p2222 phablet@127.0.0.1 \"bash -ic \'source /etc/profile; cordova-ubuntu-2.8 --desktop_file_hint=/home/phablet/%FOLDERNAME%/%FOLDERNAME%.desktop /home/phablet/%FOLDERNAME%\'\"",
                               NULL };

static void printToOutputPane(QString msg) {
    QString timestamp = QDateTime::currentDateTime().toString(QString::fromLatin1("HH:mm:ss"));
    Core::ICore::instance()->messageManager()->printToOutputPane(QString(QLatin1String("[%0] %1")).arg(timestamp).arg(msg),
                                                                 Core::MessageManager::ModeSwitch);
}

bool CordovaUbuntuProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
    Q_UNUSED(arguments)

    Core::MimeDatabase *mimeDB = Core::ICore::mimeDatabase();

    const QLatin1String mimetypesXml(":/cordovaubuntuproject/CordovaUbuntuProject.mimetypes.xml");

    if (!mimeDB->addMimeTypes(mimetypesXml, errorMessage))
        return false;

    addAutoReleasedObject(new CProjectManager);
    addAutoReleasedObject(new CRunConfigurationFactory);
    addAutoReleasedObject(new CRunControlFactory);

    m_actionMenu = new QAction(QLatin1String("Run CordovaUbuntu app on Ubuntu Phone"), this);
    Core::Command *cmd = Core::ActionManager::registerAction(m_actionMenu, Core::Id("CordovaUbuntu.TOTTTT"), Core::Context(Core::Constants::C_GLOBAL));
    connect(m_actionMenu, SIGNAL(triggered()), this, SLOT(menuItemTriggered()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),SIGNAL(updateRunActions()),this,SLOT(slotUpdateActions()));

    Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT)->addAction(cmd,Core::Id(ProjectExplorer::Constants::G_BUILD_RUN));

    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString,int)));
    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));

    return true;
}

void CordovaUbuntuProjectManagerPlugin::slotUpdateActions() {
    ProjectExplorer::ProjectExplorerPlugin* projectExplorerInstance = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project* currentProject = projectExplorerInstance->currentProject();
    if (currentProject == NULL || !qobject_cast<CProject *>(currentProject)) {
        m_actionMenu->setEnabled(false);
        return;
    }

    m_actionMenu->setEnabled(currentProject->projectManager()->mimeType() == QLatin1String(CORDOVAUBUNTUPROJECT_MIMETYPE));
}

void CordovaUbuntuProjectManagerPlugin::onStarted(QString cmd) {
    printToOutputPane(QString::fromLatin1("Started %0").arg(cmd));
}

void CordovaUbuntuProjectManagerPlugin::onMessage(QString msg) {
    printToOutputPane(msg);
}

void CordovaUbuntuProjectManagerPlugin::onError(QString msg) {
    printToOutputPane(QString::fromLatin1("%0").arg(msg));
}

void CordovaUbuntuProjectManagerPlugin::onFinished(QString cmd, int code) {
    printToOutputPane(QString::fromLatin1("%0 finished with code %1").arg(cmd).arg(code));
}

void CordovaUbuntuProjectManagerPlugin::menuItemTriggered() {
    if (m_ubuntuProcess.state() != QProcess::NotRunning) {
        m_ubuntuProcess.stop();
    }

    ProjectExplorer::Project* project = NULL;

    project = ProjectExplorer::ProjectExplorerPlugin::instance()->currentProject();
    if (project == NULL || !qobject_cast<CProject *>(project)) {
        QMessageBox::information(Core::ICore::mainWindow(),QLatin1String(Ubuntu::Constants::UBUNTU_DIALOG_NO_PROJECTOPEN_TITLE),
                                 QLatin1String(Ubuntu::Constants::UBUNTU_DIALOG_NO_PROJECTOPEN_TEXT));
        return;
    }

    QString projectDirectory = project->projectDirectory();
    QString folderName = QFileInfo(projectDirectory).baseName();

    for (int i = 0; RUN_ON_DEVICE[i]; i++) {
        QStringList cmdList;
        QString command = QString(QLatin1String(RUN_ON_DEVICE[i])).replace(QLatin1String(FOLDERNAME),folderName);
        cmdList << command << projectDirectory;
        m_ubuntuProcess.append(cmdList);
    }

    m_ubuntuProcess.start(QLatin1String("Run CordovaUbuntu on Device"));
}

}
