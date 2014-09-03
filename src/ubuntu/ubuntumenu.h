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

#ifndef UBUNTUMENU_H
#define UBUNTUMENU_H

#include <coreplugin/actionmanager/actionmanager.h>


#include "ubuntuprocess.h"
#include "ubuntuconstants.h"

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonDocument>
#include <QPointer>

namespace ProjectExplorer {
class Project;
}

namespace Ubuntu {
namespace Internal {


class UbuntuMenu : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuMenu(QObject *parent = 0);

    void initialize();

    static UbuntuMenu* instance();
    static QAction* menuAction(const Core::Id& id);

    void parseMenu(QJsonObject obj, Core::ActionContainer*& parent, const Core::Id &group = Core::Id());

    QString menuPath(QString fileName);
    QJsonDocument getMenuJSON();

public slots:
    void slotUpdateActions();

signals:
    void finished_action(QString);
    void finished_action(const QProcess* process, QString cmd);
    void requestBuildAndInstallProject (); //triggered from menu.json
    void requestBuildAndVerifyProject ();  //triggered from menu.json
    void requestBuildProject ();           //triggered from menu.json
    
protected slots:
    void menuItemTriggered();
    void onStarted(QString);
    void onMessage(QString);
    void onError(QString);
    void onFinished(QString cmd, int code);
    void onFinished(const QProcess* programm, QString cmd, int code);
    void createManifestFile();
    void setContextMenuProject(ProjectExplorer::Project* p);

protected:
    typedef QList<QJsonValue> QJsonValueList;

    QJsonDocument m_obj;
    QMap<QString,QJsonValueList> m_commandMap;

    UbuntuProcess m_ubuntuProcess;

    QMap<Core::Id,QAction*> m_actions;

private:
    bool isProperUbuntuHtmlProject(ProjectExplorer::Project *project) const;
    static UbuntuMenu *m_instance;
    QPointer<ProjectExplorer::Project> m_ctxMenuProject;
};


} // Internal
} // Ubuntu


#endif // UBUNTUMENU_H
