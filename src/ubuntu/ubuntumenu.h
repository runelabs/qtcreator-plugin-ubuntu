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

    void parseMenu(QJsonObject obj, Core::ActionContainer*& parent, const Core::Id &group = Core::Id());

    QString menuPath(QString fileName);
    QJsonDocument getMenuJSON();

public slots:
    void slotUpdateActions();
    
protected slots:
    void menuItemTriggered();
    void onStarted(QString);
    void onMessage(QString);
    void onError(QString);
    void onFinished(QString cmd, int code);

protected:
    typedef QList<QJsonValue> QJsonValueList;

    QJsonDocument m_obj;
    QMap<QString,QJsonValueList> m_commandMap;

    UbuntuProcess m_ubuntuProcess;

    QList<QAction*> m_actions;

private:
    bool isProperUbuntuHtmlProject(ProjectExplorer::Project *project) const;
};


} // Internal
} // Ubuntu


#endif // UBUNTUMENU_H
