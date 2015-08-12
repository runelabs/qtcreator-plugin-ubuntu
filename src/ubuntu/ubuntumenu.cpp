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
#include "ubuntudevicemode.h"
#include "ubuntuproject.h"
#include "ubuntuclicktool.h"
#include "ubuntudevice.h"
#include "ubuntuclickmanifest.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/messagemanager.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/compileoutputwindow.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projecttree.h>
#include <utils/qtcprocess.h>
#include <ssh/sshconnection.h>

#include <QProcessEnvironment>
#include <QAction>
#include <QMenu>
#include <QInputDialog>
#include <QDebug>
#include <QVariantMap>
#include <QMessageBox>

#include <QJsonObject>
#include <QJsonArray>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

enum {
    debug = 0
};

UbuntuMenu *UbuntuMenu::m_instance = 0;

UbuntuMenu *UbuntuMenu::instance()
{
    return m_instance;
}

QAction *UbuntuMenu::menuAction(const Core::Id &id)
{
    UbuntuMenu* men = UbuntuMenu::instance();
    if(men->m_actions.contains(id))
        return men->m_actions[id];
    return 0;
}

UbuntuMenu::UbuntuMenu(QObject *parent) :
    QObject(parent)
{
    m_instance = this;
    m_obj = getMenuJSON();

    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString,int)));
    connect(&m_ubuntuProcess,SIGNAL(finished(const QProcess*,QString,int)),this,SLOT(onFinished(const QProcess*,QString,int)));
    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),SIGNAL(updateRunActions()),this,SLOT(slotUpdateActions()));
    connect(UbuntuDeviceMode::instance(),SIGNAL(updateDeviceActions()),this,SLOT(slotUpdateActions()));

    ProjectExplorer::ProjectTree *ptree = ProjectExplorer::ProjectTree::instance();
    connect(ptree, SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)),
            this, SLOT(setContextMenuProject(ProjectExplorer::Project*)));
}


void UbuntuMenu::slotUpdateActions() {
    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();
    bool isQmlProject = false;
    bool isQmakeProject = false;
    bool isUbuntuProject = false;
    bool isUbuntuHtmlProject = false;
    bool isClickTarget = false;

    if (startupProject) {
        isQmlProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::QMLPROJECT_MIMETYPE));
        isQmakeProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::QMAKE_MIMETYPE));
        isUbuntuProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE));
        isUbuntuHtmlProject = isProperUbuntuHtmlProject(startupProject);
        isUbuntuProject = isUbuntuProject || isUbuntuHtmlProject || isQmlProject;
        isClickTarget = startupProject->activeTarget()
                && startupProject->activeTarget()->kit()
                && ProjectExplorer::ToolChainKitInformation::toolChain(startupProject->activeTarget()->kit())
                && ProjectExplorer::ToolChainKitInformation::toolChain(startupProject->activeTarget()->kit())->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID);
    }

    //bool canRun = projectExplorerInstance->canRun(startupProject,ProjectExplorer::NormalRunMode);
    bool projectOpen = (startupProject!=NULL);
    bool deviceDetected = !UbuntuDeviceMode::instance()->device().isNull();

    foreach(QAction* act, m_actions) {
        bool requiresDevice = act->property(Constants::UBUNTU_MENUJSON_DEVICEREQUIRED).toBool();
        bool requiresProject = act->property(Constants::UBUNTU_MENUJSON_PROJECTREQUIRED).toBool();
        bool requiresQmlProject = act->property(Constants::UBUNTU_MENUJSON_QMLPROJECTREQUIRED).toBool();
        bool requiresQmakeProject = act->property(Constants::UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED).toBool();
        bool requiresUbuntuProject = act->property(Constants::UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED).toBool();
        bool requiresUbuntuHtmlProject = act->property(Constants::UBUNTU_MENUJSON_UBUNTUHTMLPROJECTREQUIRED).toBool();
        bool requiresClickToolchain = act->property(Constants::UBUNTU_MENUJSON_CLICKTOOLCHAINREQUIRED).toBool();
        bool actionEnabled = ( (requiresQmakeProject ? isQmakeProject : true) &&
                               (requiresQmlProject ? isQmlProject : true) &&
                               (requiresUbuntuHtmlProject ? isUbuntuHtmlProject : true) &&
                               (requiresDevice ? deviceDetected : true) &&
                               (requiresProject ? projectOpen : true) &&
                               (requiresUbuntuProject ? isUbuntuProject : true) &&
                               (requiresClickToolchain ? isClickTarget : true) );
        act->setEnabled( actionEnabled );
    }
}

void UbuntuMenu::onStarted(QString cmd) {
    printToOutputPane(QString::fromLatin1(Constants::UBUNTUMENU_ONSTARTED).arg(cmd));
}

void UbuntuMenu::onMessage(QString msg) {
    printToOutputPane(msg);
}

void UbuntuMenu::onError(QString msg) {
    printToOutputPane(QString::fromLatin1(Constants::UBUNTUMENU_ONERROR).arg(msg));
}

void UbuntuMenu::onFinished(QString cmd, int code) {
    emit finished_action(cmd);
    printToOutputPane(QString::fromLatin1(Constants::UBUNTUMENU_ONFINISHED).arg(cmd).arg(code));
}

void UbuntuMenu::onFinished(const QProcess *programm, QString cmd, int)
{
    emit finished_action(programm,cmd);
}

void UbuntuMenu::createManifestFile()
{
    if(Q_UNLIKELY(!m_ctxMenuProject))
        return;

    bool changed = false;

    QString manifestFilePath = m_ctxMenuProject->projectDirectory().toString()
            + QDir::separator()
            + QLatin1String("manifest.json");

    UbuntuClickManifest manifest;
    if(QFile::exists(manifestFilePath)) {
        if(!manifest.load(manifestFilePath)) {
            QMessageBox::warning(Core::ICore::mainWindow(),tr("Error"),
                                 tr("The manifest.json file already exists, but can not be opened."));
            return;
        }
    } else {
        if(!manifest.load(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MANIFEST),m_ctxMenuProject.data())) {
            QMessageBox::warning(Core::ICore::mainWindow(),tr("Error"),
                                 tr("Could not open the manifest.json template"));
            return;
        }

        changed = true;
        manifest.setFileName(manifestFilePath);
        manifest.save();
    }

    QList<UbuntuClickManifest::Hook> hooks = manifest.hooks();
    foreach(const UbuntuClickManifest::Hook &hook, hooks) {
        if(!hook.appArmorFile.isEmpty()) {
            UbuntuClickManifest aaFile;
            QString aaFilePath = QDir::cleanPath(m_ctxMenuProject->projectDirectory().toString()
                                                 + QDir::separator()
                                                 + hook.appArmorFile);
            if(QFile::exists(aaFilePath))
                continue;

            if(!aaFile.load(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MYAPP))) {
                printToOutputPane(tr("Could not open the apparmor template"));
                continue;
            }
            changed = true;
            aaFile.setFileName(aaFilePath);
            aaFile.save();
        }
    }

    QMessageBox::information(Core::ICore::mainWindow(),
                             tr("Files created"),
                             changed ? tr("The manifest.json and apparmor files have been created in the project directory.\nPlease make sure to add them to your project file.")
                                     : tr("All required files already exist in your project directory"));
}

void UbuntuMenu::setContextMenuProject(ProjectExplorer::Project *p)
{
    m_ctxMenuProject = p;
}

QString UbuntuMenu::menuPath(QString fileName) {
    return Constants::UBUNTU_MENUPATH + fileName;
}

QJsonDocument UbuntuMenu::getMenuJSON() {
    QByteArray contents;
    QString errorMsg;
    if (readFile(menuPath(QLatin1String(Constants::UBUNTU_MENUJSON)),&contents, &errorMsg) == false) {
        qWarning() << __PRETTY_FUNCTION__ << errorMsg;
    }
    QJsonParseError error;
    QJsonDocument retval = QJsonDocument::fromJson(contents,&error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << error.errorString();
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
        QStringList contexts = QStringList()<<QLatin1String(Core::Constants::C_GLOBAL);
        bool actionProjectRequired = false;
        bool actionDeviceRequired = false;
        bool actionQmlProjectRequired = false;
        bool actionQmakeProjectRequired = false;
        bool actionUbuntuProjectRequired = false;
        bool actionUbuntuHtmlProjectRequired = false;
        bool actionSaveRequired = false;
        bool clickTargetRequired = false;
        bool clickToolchainRequired = false;

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
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_UBUNTUHTMLPROJECTREQUIRED))) {
            actionUbuntuHtmlProjectRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_UBUNTUHTMLPROJECTREQUIRED)).toBool();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_SAVEREQUIRED))) {
            actionSaveRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_SAVEREQUIRED)).toBool();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_CONTEXT))) {
            //contexts can contains either a string, or a array of strings
            QJsonValue v = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_CONTEXT));
            if(v.isArray()) {
                QJsonArray jsonContexts = v.toArray();

                QStringList tmp_contexts;
                for (int i = 0; i < jsonContexts.size(); i++) {
                    tmp_contexts.append(jsonContexts.at(i).toString());
                }

                if(!tmp_contexts.isEmpty())
                    contexts = tmp_contexts;

            } else {
                contexts = QStringList()<<obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_CONTEXT)).toString();
            }
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_CLICKTARGETREQUIRED))) {
            clickTargetRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_CLICKTARGETREQUIRED)).toBool();
        }
        if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_CLICKTOOLCHAINREQUIRED))) {
            clickToolchainRequired = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_CLICKTOOLCHAINREQUIRED)).toBool();
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
                m_commandMap.insert(actionId,actionsList);
            }
        }

        QAction *act= new QAction(actionName, this);
        act->setObjectName(actionId);
        act->setProperty(Constants::UBUNTU_MENUJSON_PROJECTREQUIRED,actionProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_DEVICEREQUIRED,actionDeviceRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED,actionQmakeProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_QMLPROJECTREQUIRED,actionQmlProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED,actionUbuntuProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_UBUNTUHTMLPROJECTREQUIRED,actionUbuntuHtmlProjectRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_SAVEREQUIRED,actionSaveRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_CLICKTARGETREQUIRED,clickTargetRequired);
        act->setProperty(Constants::UBUNTU_MENUJSON_CLICKTOOLCHAINREQUIRED,clickToolchainRequired);

        connect(act, SIGNAL(triggered()), this, SLOT(menuItemTriggered()));
        m_actions.insert(Core::Id(actionId.toUtf8().constData()),act);

        foreach(const QString& context,contexts) {
            Core::Command *cmd = Core::ActionManager::registerAction(act, Core::Id(actionId.toUtf8().constData()), Core::Context(context.toUtf8().constData()));
            if (actionKeySequence.isEmpty() == false) {
                cmd->setDefaultKeySequence(QKeySequence(actionKeySequence));
            }
            if (actionWorkingDirectory.isEmpty() == false) {
                act->setProperty(Constants::UBUNTU_MENUJSON_WORKINGDIRECTORY,actionWorkingDirectory);
            }

            //hide if the context does not match creators current context
            cmd->setAttribute(Core::Command::CA_Hide);

            if (parent == NULL) {
                qWarning() << Constants::ERROR_MSG_NO_MENU_DEFINED;
            } else {
                parent->addAction(cmd,group);
            }
        }
    }

}

bool UbuntuMenu::isProperUbuntuHtmlProject(ProjectExplorer::Project *project) const
{
    if (Q_UNLIKELY(NULL == project))
        return false;

    UbuntuProject* ubuntuProject = qobject_cast<UbuntuProject*>(project);
    if (NULL == ubuntuProject)
        return false;

    return ubuntuProject->filesFileName().endsWith(QLatin1String(Constants::UBUNTUHTMLPROJECT_SUFFIX));
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
        UbuntuClickManifest manifest;

        if (projectRequired.isValid() && projectRequired.toBool() == true) {
            project = ProjectExplorer::SessionManager::startupProject();

            if (project == NULL) {
                QMessageBox::information(Core::ICore::mainWindow(),QLatin1String(Constants::UBUNTU_DIALOG_NO_PROJECTOPEN_TITLE),QLatin1String(Constants::UBUNTU_DIALOG_NO_PROJECTOPEN_TEXT));
                return;
            } else {
                QString manifestPath = project->projectDirectory()
                        .appendPath(QStringLiteral("manifest.json")).toString();

                if (!manifest.load(manifestPath)) {
                    qWarning() << "Could not load manifest from path" << manifestPath;
                }
            }
        }

        QVariant saveModifiedFilesRequired = act->property(Constants::UBUNTU_MENUJSON_SAVEREQUIRED);
        if (saveModifiedFilesRequired.isValid() && saveModifiedFilesRequired.toBool()==true) {
            ProjectExplorer::ProjectExplorerPlugin::instance()->saveModifiedFiles();
        }

        if (m_commandMap.contains(act->objectName())) {
            QJsonValueList actions = m_commandMap.value(act->objectName());

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
                        //check if metacall
                    } else if (obj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_METACALL))) {
                        QJsonValue metaCall = obj.value(QLatin1String(Constants::UBUNTU_MENUJSON_METACALL));
                        if (metaCall.isObject()) {
                            QJsonObject metaCallObj = metaCall.toObject();
                            QByteArray methodName;
                            QList<QGenericArgument> args;
                            if(!metaCallObj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_METACALL_METHOD))) {
                                qWarning()<<"Metacall menuitem does not contain a method name";
                                return;
                            }

                            methodName = metaCallObj[QLatin1String(Constants::UBUNTU_MENUJSON_METACALL_METHOD)].toString().toLocal8Bit();
                            if(methodName.isEmpty()){
                                qWarning()<< "Property method of a metacall menuitem has to be a string and can not be empty";
                                return;
                            }

                            //we first need to fill a list of variant values, otherwise we get dangling pointers in QGenericArgument
                            //because of QVariants getting out of scope
                            QVariantList varArgs;
                            if(metaCallObj.contains(QLatin1String(Constants::UBUNTU_MENUJSON_METACALL_ARGS))) {
                                QJsonArray arr = metaCallObj[QLatin1String(Constants::UBUNTU_MENUJSON_METACALL_ARGS)].toArray();
                                foreach(const QJsonValue &val,arr) {
                                    switch(val.type()) {
                                        case QJsonValue::Undefined:
                                        case QJsonValue::Null: {
                                            qWarning()<<"Arguments of a metacall can not be null";
                                            return;
                                            break;
                                        }
                                        case QJsonValue::Bool:
                                        case QJsonValue::Double:
                                        case QJsonValue::String:
                                        case QJsonValue::Array:
                                        case QJsonValue::Object: {
                                            varArgs.append(val.toVariant());
                                            break;
                                        }
                                    }
                                }
                            }

                            foreach(const QVariant &val,varArgs)
                                args.append(QGenericArgument(val.typeName(),val.data()));

                            bool ok = false;
                            int argsCount = args.size();
                            if (argsCount > 9) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)),
                                                               (args.at(3)),
                                                               (args.at(4)),
                                                               (args.at(5)),
                                                               (args.at(6)),
                                                               (args.at(7)),
                                                               (args.at(8)),
                                                               (args.at(9)));
                            } else if (argsCount > 8) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)),
                                                               (args.at(3)),
                                                               (args.at(4)),
                                                               (args.at(5)),
                                                               (args.at(6)),
                                                               (args.at(7)),
                                                               (args.at(8)));
                            } else if (argsCount > 7) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)),
                                                               (args.at(3)),
                                                               (args.at(4)),
                                                               (args.at(5)),
                                                               (args.at(6)),
                                                               (args.at(7)));
                            } else if (argsCount > 6) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)),
                                                               (args.at(3)),
                                                               (args.at(4)),
                                                               (args.at(5)),
                                                               (args.at(6)));
                            } else if (argsCount > 5) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)),
                                                               (args.at(3)),
                                                               (args.at(4)),
                                                               (args.at(5)));
                            } else if (argsCount > 4) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)),
                                                               (args.at(3)),
                                                               (args.at(4)));
                            } else if (argsCount > 3) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)),
                                                               (args.at(3)));
                            } else if (argsCount > 2) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)),
                                                               (args.at(2)));
                            } else if (argsCount > 1) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)),
                                                               (args.at(1)));
                            } else if (argsCount > 0) {
                                ok = QMetaObject::invokeMethod(this,methodName.data(),
                                                               (args.at(0)));
                            } else {
                                ok = QMetaObject::invokeMethod(this,methodName.data());
                            }

                            if(!ok)
                                qWarning()<<"Invoke of "<<methodName<<" with arguments "<<varArgs<<" failed";
                            return;
                        }
                    }
                    // check if command
                } else if (value.isString()) {
                    QString command = value.toString();
                    QString workingDirectory;

                    if (project) {

                        QString projectDirectory = project->projectDirectory().toString();
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

                        if (manifest.isInitialized()) {
                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_NAME_FROM_MANIFEST),manifest.name());
                        }

                        if (isProperUbuntuHtmlProject(project)) {
                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_APP_RUNNER_EXECNAME),
                                                      QLatin1String(Constants::UBUNTUHTML_PROJECT_LAUNCHER_EXE));
                        }

                        QVariant clickTargetRequired = act->property(Constants::UBUNTU_MENUJSON_CLICKTARGETREQUIRED);
                        if(clickTargetRequired.isValid() && clickTargetRequired.toBool()) {
                            UbuntuClickTool::Target clickTarget;
                            if(!UbuntuClickTool::getTargetFromUser(&clickTarget))
                                return;

                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_CLICK_PACKAGING_FOLDER)
                                                      ,QString::fromLatin1(Constants::UBUNTU_CLICK_CLICK_PACKAGE_DIR));
                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_CLICK_ARCH),clickTarget.architecture);
                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_CLICK_FRAMEWORK),clickTarget.framework);
                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_CLICK_SERIES),clickTarget.series);

                            //this is a clicktarget, so we change the builddirectory to the current active buildconfig
                            //directory
                            ProjectExplorer::Target* qtcTarget = project->activeTarget();
                            if(!qtcTarget)
                                return;

                            ProjectExplorer::BuildConfiguration* qtcBuildConfig = qtcTarget->activeBuildConfiguration();
                            if(!qtcBuildConfig)
                                return;

                            workingDirectory = qtcBuildConfig->buildDirectory().toString();
                        }

                        if(project->id() == Constants::GO_PROJECT_ID) {
                            QVariant ret(QVariant::String);
                            QGenericReturnArgument genRet(ret.typeName(),ret.data());
                            if(QMetaObject::invokeMethod(project,"applicationNames",genRet)) {
                                command = command.replace(QLatin1String(Constants::UBUNTU_GO_BUILD_TARGETS),ret.toString());
                            }
                        }
                    }

                    bool requiresDevice = act->property(Constants::UBUNTU_MENUJSON_DEVICEREQUIRED).toBool();
                    if(requiresDevice) {
                        UbuntuDevice::ConstPtr device = UbuntuDeviceMode::instance()->device();
                        if (device) {
                            if( device->deviceState() != ProjectExplorer::IDevice::DeviceReadyToUse ) {
                                QMessageBox::warning(Core::ICore::mainWindow(),tr("Device not ready"),tr("The currently selected device is not ready, please select another one on the devices mode"));
                                return;
                            }

                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_DEVICE_IP),device->sshParameters().host);
                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_DEVICE_USERNAME),device->sshParameters().userName);
                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_DEVICE_PORT),QString::number(device->sshParameters().port));

                            command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_SERIALNUMBER),device->serialNumber());
                        }
                    }
                    command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_SHAREDIRECTORY),Constants::UBUNTU_SHAREPATH);
                    command = command.replace(QLatin1String(Constants::UBUNTU_ACTION_SCRIPTDIRECTORY),Constants::UBUNTU_SCRIPTPATH);
                    if (bQuery && bQueryOk) {
                        command = QString(command).arg(queryData);
                    }

                    QStringList cmdList;
                    cmdList << command << workingDirectory;

                    if(debug) qDebug()<<command;

                    m_ubuntuProcess.append(cmdList);
                }
            }
            m_ubuntuProcess.start(act->text());
        } else {
            qWarning() << __PRETTY_FUNCTION__  << Constants::ERROR_MSG_NOACTIONS;
        }
    } else {
        qWarning() << __PRETTY_FUNCTION__  << Constants::ERROR_MSG_COULD_NOT_CAST_TO_ACTION;
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
        qWarning() << Constants::ERROR_MSG_UNABLE_TO_PARSE_MENUJSON;
    }
}
