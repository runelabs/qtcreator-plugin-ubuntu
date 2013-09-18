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

#include "ubuntumenu.h"
#include "ubuntushared.h"
#include "ubuntuconstants.h"
#include "ubuntudeviceswidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/messagemanager.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/compileoutputwindow.h>
#include <projectexplorer/iprojectmanager.h>
#include <utils/qtcprocess.h>

#include <QProcessEnvironment>
#include <QAction>
#include <QMenu>
#include <QInputDialog>
#include <QDebug>
#include <QMessageBox>

#include <QJsonObject>
#include <QJsonArray>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

UbuntuMenu::UbuntuMenu(QObject *parent) :
    QObject(parent)
{
    m_obj = getMenuJSON();

    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString,int)));
    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),SIGNAL(updateRunActions()),this,SLOT(slotUpdateActions()));
    connect(UbuntuDevicesWidget::instance(),SIGNAL(updateDeviceActions()),this,SLOT(slotUpdateActions()));
}


void UbuntuMenu::slotUpdateActions() {
    ProjectExplorer::ProjectExplorerPlugin* projectExplorerInstance = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project* startupProject = projectExplorerInstance->startupProject();
    bool isQmlProject = false;
    bool isQmakeProject = false;
    bool isUbuntuProject = false;
    bool isCordovaProject = false;


    if (startupProject) {
        isQmlProject = (startupProject->projectManager()->mimeType() == QLatin1String("application/x-qmlproject"));
        isQmakeProject = (startupProject->projectManager()->mimeType() == QLatin1String("application/vnd.qt.qmakeprofile"));
        isCordovaProject = (startupProject->projectManager()->mimeType() == QLatin1String("application/x-cordovaproject"));
        isUbuntuProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE));
    }

    //bool canRun = projectExplorerInstance->canRun(startupProject,ProjectExplorer::NormalRunMode);
    bool projectOpen = (startupProject!=NULL);
    bool deviceDetected = UbuntuDevicesWidget::instance()->deviceDetected();

    foreach(QAction* act, m_actions) {
        bool requiresDevice = act->property(Constants::UBUNTU_MENUJSON_DEVICEREQUIRED).toBool();
        bool requiresProject = act->property(Constants::UBUNTU_MENUJSON_PROJECTREQUIRED).toBool();
        bool requiresQmlProject = act->property(Constants::UBUNTU_MENUJSON_QMLPROJECTREQUIRED).toBool();
	bool requiresCordovaProject = act->property(Constants::UBUNTU_MENUJSON_CORDOVAPROJECTREQUIRED).toBool();
        bool requiresQmakeProject = act->property(Constants::UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED).toBool();
        bool requiresUbuntuProject = act->property(Constants::UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED).toBool();
        bool actionEnabled = ( (requiresQmakeProject ? isQmakeProject : true) && (requiresQmlProject ? isQmlProject : true) && (requiresDevice ? deviceDetected : true) && (requiresProject ? projectOpen : true) && (requiresUbuntuProject ? isUbuntuProject : true) && (requiresCordovaProject ? isCordovaProject : true));

        act->setEnabled( actionEnabled );
    }
}

void UbuntuMenu::onStarted(QString cmd) {
    printToOutputPane(QString::fromLatin1("Started %0").arg(cmd));
}

void UbuntuMenu::onMessage(QString msg) {
    printToOutputPane(msg);
}

void UbuntuMenu::onError(QString msg) {
    printToOutputPane(QString::fromLatin1("%0").arg(msg));
}

void UbuntuMenu::onFinished(QString cmd, int code) {
    printToOutputPane(QString::fromLatin1("%0 finished with code %1").arg(cmd).arg(code));
}

QString UbuntuMenu::menuPath(QString fileName) {
    return Constants::UBUNTU_MENUPATH + fileName;
}

QJsonDocument UbuntuMenu::getMenuJSON() {
    QByteArray contents;
    QString errorMsg;
    if (readFile(menuPath(QLatin1String(Constants::UBUNTU_MENUJSON)),&contents, &errorMsg) == false) {
        qDebug() << __PRETTY_FUNCTION__ << errorMsg;
    }
    QJsonParseError error;
    QJsonDocument retval = QJsonDocument::fromJson(contents,&error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << error.errorString();
    }
    return retval;
}

void UbuntuMenu::parseMenu(QJsonObject obj, Core::ActionContainer*& parent, const Core::Id &group) {
    if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_SUBMENU))) {
        QString menuName, menuId;
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_NAME))) {
            menuName = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_NAME)).toString();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_ID))) {
            menuId = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_ID)).toString();
        }
        Core::ActionContainer *actionContainer = Core::ActionManager::createMenu(Core::Id(menuId.toUtf8().constData()));
        actionContainer->menu()->setTitle(menuName);
        actionContainer->menu()->setObjectName(menuId);

        QJsonValue submenu = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_SUBMENU));
        if (submenu.isArray()) {
            QJsonArray submenuArray = submenu.toArray();
            for (int idx=0; idx<submenuArray.size(); idx++) {
                QJsonValue subMenuItem = submenuArray.at(idx);
                if (subMenuItem.isObject()) {
                    parseMenu(subMenuItem.toObject(),actionContainer);
                }
            }
        }

        if (parent == NULL) {
            parent = actionContainer;
        } else {
            parent->addMenu(actionContainer,group);
        }
    } else if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_ACTIONS))) {
        QString actionName;
        QString actionId;
        QString actionKeySequence;
        QString actionWorkingDirectory;
        bool actionProjectRequired = false;
        bool actionDeviceRequired = false;
        bool actionQmlProjectRequired = false;
        bool actionQmakeProjectRequired = false;
        bool actionUbuntuProjectRequired = false;
        bool actionSaveRequired = false;

        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_NAME))) {
            actionName = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_NAME)).toString();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_ID))) {
            actionId = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_ID)).toString();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_KEYSEQUENCE))) {
            actionKeySequence = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_KEYSEQUENCE)).toString();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_WORKINGDIRECTORY))) {
            actionWorkingDirectory = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_WORKINGDIRECTORY)).toString();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_PROJECTREQUIRED))) {
            actionProjectRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_PROJECTREQUIRED)).toBool();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_DEVICEREQUIRED))) {
            actionDeviceRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_DEVICEREQUIRED)).toBool();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_QMLPROJECTREQUIRED))) {
            actionQmlProjectRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_QMLPROJECTREQUIRED)).toBool();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED))) {
            actionQmakeProjectRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED)).toBool();
        }

        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED))) {
            actionUbuntuProjectRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED)).toBool();
        }

        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_SAVEREQUIRED))) {
            actionSaveRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_SAVEREQUIRED)).toBool();
        }

        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_ACTIONS)) && obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_ACTIONS)).isArray()) {

            QJsonValue actions = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_ACTIONS));
            if (actions.isArray()) {
                QJsonArray actionsArray = actions.toArray();
                QJsonValueList actionsList;
                for (int idx=0; idx<actionsArray.size(); idx++) {
                    QJsonValue actionItem = actionsArray.at(idx);
                    actionsList.append(actionItem);
                }
                m_commandMap.insert(actionName,actionsList);
            }
        }

        QAction *act= new QAction(actionName, this);
        act->setObjectName(actionId);

        Core::Command *cmd = Core::ActionManager::registerAction(act, Core::Id(actionId.toUtf8().constData()), Core::Context(Core::Constants::C_GLOBAL));
        if (actionKeySequence.isEmpty() == false) {
            cmd->setDefaultKeySequence(QKeySequence(actionKeySequence));
        }
        if (actionWorkingDirectory.isEmpty() == false) {
            act->setProperty(Constants::UBUNTU_MENUJSON_WORKINGDIRECTORY,actionWorkingDirectory);
        }

        act->setProperty(Constants::UBUNTU_MENUJSON_PROJECTREQUIRED,actionProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_DEVICEREQUIRED,actionDeviceRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED,actionQmakeProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_QMLPROJECTREQUIRED,actionQmlProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED,actionUbuntuProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_SAVEREQUIRED,actionSaveRequired);

        connect(act, SIGNAL(triggered()), this, SLOT(menuItemTriggered()));
        m_actions.append(act);

        if (parent == NULL) {
            qWarning() << "No menu defined";
        } else {
            parent->addAction(cmd,group);
        }
    }

}

void UbuntuMenu::menuItemTriggered() {
    QAction* act = qobject_cast<QAction*>(sender());
    if (act) {

        // if we are executing something now, then kill it!
        if (m_ubuntuProcess.state() != QProcess::NotRunning) {
            m_ubuntuProcess.stop();
        }

        QVariant projectRequired = act->property(Constants::UBUNTU_MENUJSON_PROJECTREQUIRED);
        ProjectExplorer::Project* project = NULL;

        if (projectRequired.isValid() && projectRequired.toBool() == true) {
            project = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();

            if (project == NULL) {
                QMessageBox::information(Core::ICore::mainWindow(),QLatin1String(Constants::UBUNTU_DIALOG_NO_PROJECTOPEN_TITLE),QLatin1String(Constants::UBUNTU_DIALOG_NO_PROJECTOPEN_TEXT));
                return;
            }
        }

        QVariant saveModifiedFilesRequired = act->property(Constants::UBUNTU_MENUJSON_SAVEREQUIRED);
        if (saveModifiedFilesRequired.isValid() && saveModifiedFilesRequired.toBool()==true) {
            ProjectExplorer::ProjectExplorerPlugin::instance()->saveModifiedFiles();
        }

        if (m_commandMap.contains(act->text())) {
            QJsonValueList actions = m_commandMap.value(act->text());

            QString queryData;
            bool bQueryOk = false;
            bool bQuery = false;

            for (int idx=0; idx < actions.size(); idx++) {
                QJsonValue value = actions.at(idx);
                if (value.isObject()) {
                    QJsonObject obj = value.toObject();

                    // check if the object is a querydialog
                    if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_QUERYDIALOG))) {
                        QJsonValue queryDialog = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_QUERYDIALOG));
                        if (queryDialog.isObject()) {
                            QJsonObject queryDialogObj = queryDialog.toObject();
                            QString queryDialogTitle;
                            QString queryDialogMessage;
                            QString queryDialogValue;

                            if (queryDialogObj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_TITLE))) {
                                queryDialogTitle = queryDialogObj.value(QLatin1String(Constants::UBUNTU_MENUJSON_TITLE)).toString();
                            }

                            if (queryDialogObj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_MESSAGE))) {
                                queryDialogMessage = queryDialogObj.value(QLatin1String(Constants::UBUNTU_MENUJSON_MESSAGE)).toString();
                            }

                            if (queryDialogObj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_VALUE))) {
                                queryDialogValue = queryDialogObj.value(QLatin1String(Constants::UBUNTU_MENUJSON_VALUE)).toString();
                            }

                            queryData = QInputDialog::getText(Core::ICore::mainWindow(), queryDialogTitle,
                                                                     queryDialogMessage, QLineEdit::Normal,
                                                                     queryDialogValue, &bQueryOk);

                            // raise a flag that there is query data available for future actions
                            bQuery = true;

                            // if user has cancelled
                            if (bQueryOk == false) {
                                // clear queue
                                m_ubuntuProcess.clear();
                                return;
                            }
                        }
                    // check if messageDialog
                    } else if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_MESSAGEDIALOG))) {
                        QJsonValue messageDialog = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_MESSAGEDIALOG));
                        if (messageDialog.isObject()) {
                            QJsonObject messageDialogObj = messageDialog.toObject();
                            QString messageDialogTitle;
                            QString messageDialogMessage;

                            if (messageDialogObj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_TITLE))) {
                                messageDialogTitle = messageDialogObj.value(QLatin1String(Constants::UBUNTU_MENUJSON_TITLE)).toString();
                            }

                            if (messageDialogObj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_MESSAGE))) {
                                messageDialogMessage = messageDialogObj.value(QLatin1String(Constants::UBUNTU_MENUJSON_MESSAGE)).toString();
                            }

                            QMessageBox::information(Core::ICore::mainWindow(), messageDialogTitle,
                                                                     messageDialogMessage);
                        }
                    }
                // check if command
                } else if (value.isString()) {
                    QString command = value.toString();
                    QString workingDirectory;

                    if (project) {

                        QString projectDirectory = project->projectDirectory();
                        QString displayName = project->displayName();

                        QString folderName = projectDirectory;
                        // Bug 1212937 workaround
                        folderName = folderName.replace(QString(QLatin1String("%0/")).arg(QFileInfo(projectDirectory).path()),QLatin1String(""));

                        QStringList projectFiles = project->files(ProjectExplorer::Project::AllFiles);

                        QString workingDirectoryData = act->property(Constants::UBUNTU_MENUJSON_WORKINGDIRECTORY).toString();
                        if (workingDirectoryData.isEmpty() == false) {
                            workingDirectory = workingDirectoryData.arg(projectDirectory);
                        } else {
                            workingDirectory = projectDirectory;
                        }

                        command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_PROJECTDIRECTORY),projectDirectory);
                        command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_FOLDERNAME),folderName);
                        command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_DISPLAYNAME),displayName);
                        command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_PROJECTFILES),projectFiles.join(QLatin1String(" ")));
                    }

                    command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_SHAREDIRECTORY),Constants::UBUNTU_SHAREPATH);
                    command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_SCRIPTDIRECTORY),Constants::UBUNTU_SCRIPTPATH);
                    command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_SERIALNUMBER),UbuntuDevicesWidget::instance()->serialNumber());

                    if (bQuery && bQueryOk) {
                        command = QString(command).arg(queryData);
                    }

                    QStringList cmdList;
                    cmdList << command << workingDirectory;
                    m_ubuntuProcess.append(cmdList);
                }
            }
            m_ubuntuProcess.start(act->text());
        } else {
            qWarning() << __PRETTY_FUNCTION__  << "No actions defined in map";
        }
    } else {
        qWarning() << __PRETTY_FUNCTION__  << "Could not cast to action";
    }
}

void UbuntuMenu::initialize() {
    if (m_obj.isObject()) {
        QJsonObject tmp = m_obj.object();

        foreach (QString key, tmp.keys()) {
            if (tmp.contains(key)) {
                QJsonObject obj = tmp.value(key).toObject();

                Core::ActionContainer *actionContainer = NULL;

                if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_PARENT))) {
                    QString parentValue = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_PARENT)).toString();
                    if (parentValue == QLatin1String(Constants::UBUNTU_MENUJSON_PARENT_TOOLS)) actionContainer = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
                    else if (parentValue == QLatin1String(Constants::UBUNTU_MENUJSON_PARENT_EDIT)) actionContainer = Core::ActionManager::actionContainer(Core::Constants::M_EDIT);
                    else if (parentValue == QLatin1String(Constants::UBUNTU_MENUJSON_PARENT_HELP)) actionContainer = Core::ActionManager::actionContainer(Core::Constants::M_HELP);
                    else if (parentValue == QLatin1String(Constants::UBUNTU_MENUJSON_PARENT_WINDOW)) actionContainer = Core::ActionManager::actionContainer(Core::Constants::M_WINDOW);
                    else if (parentValue == QLatin1String(Constants::UBUNTU_MENUJSON_PARENT_FILE)) actionContainer = Core::ActionManager::actionContainer(Core::Constants::M_FILE);
                    else if (parentValue == QLatin1String(Constants::UBUNTU_MENUJSON_PARENT_BUILD)) actionContainer = Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
                    else if (parentValue == QLatin1String(Constants::UBUNTU_MENUJSON_PARENT_TOP)) actionContainer = Core::ActionManager::actionContainer(Core::Constants::MENU_BAR);
                    else actionContainer = Core::ActionManager::actionContainer(Core::Id(parentValue.toUtf8().constData()));
                } else {
                    actionContainer = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
                }

                QString group;
                if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_GROUP))) {
                    group = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_GROUP)).toString();
                }

                Core::Id groupId;
                if (!group.isEmpty()) {
                    groupId = Core::Id(group.toUtf8().constData());
                }
                parseMenu(obj,actionContainer,groupId);
            }
        }
    } else {
        qDebug() << "json is not valid";
    }
}
