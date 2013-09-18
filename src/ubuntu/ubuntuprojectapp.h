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

#ifndef UBUNTUPROJECTAPP_H
#define UBUNTUPROJECTAPP_H

#include <QObject>
#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>
#include <qt4projectmanager/qt4project.h>
#include <qmlprojectmanager/qmlproject.h>
#include <extensionsystem/pluginmanager.h>

#include <QJsonObject>
#include "ubuntuconstants.h"
#include "ubuntubzr.h"

namespace Ubuntu {
namespace Internal {
class UbuntuProjectApp : public QObject
{
    Q_OBJECT

public:
    UbuntuProjectApp(QObject *parent = 0);
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage);
    
    QString projectType() { return m_projectType; }
    void setProjectType(QString projectType) { m_projectType = projectType; }

    QString projectFileName() { return m_projectFileName; }
    void setProjectFileName(QString projectFileName) { m_projectFileName = projectFileName; }

    void setData(QJsonObject obj) { m_obj = obj; }
    Core::BaseFileWizardParameters parameters(QJsonObject params);

    QByteArray processReservedWords(QByteArray data, QString projectPath, QString projectName);
    QString processReservedWordsInFileName(QString data, QString projectName) { return QString::fromLatin1(processReservedWordsInFileName(data.toLatin1(),projectName)); }
    QByteArray processReservedWordsInFileName(QByteArray data, QString projectName);

    Core::Feature requiredFeature();

protected:
    QString m_projectType;
    QString m_projectFileName;


    UbuntuBzr m_bzrInfo;
    QJsonObject m_obj;
};

}
}

#endif // UBUNTUPROJECTAPP_H
