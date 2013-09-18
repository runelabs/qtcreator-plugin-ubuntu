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

#ifndef UBUNTUCLICKMANIFEST_H
#define UBUNTUCLICKMANIFEST_H

#include <QObject>
#include <QScriptProgram>
#include <QScriptEngine>
#include <QStringList>

namespace Ubuntu {
namespace Internal {

class UbuntuClickManifest : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuClickManifest(QObject *parent = 0);

signals:
    void nameChanged();
    void maintainerChanged();
    void titleChanged();
    void policyGroupsChanged();
    void versionChanged();
    void descriptionChanged();
    void saved();
    void loaded();
    void error();
    void fileNameChanged(QString);

public slots:
    void setName(QString name);
    QString name();

    void setMaintainer(QString maintainer);
    QString maintainer();

    void setTitle(QString title);
    QString title();

    void setVersion(QString version);
    QString version();

    void setDescription(QString description);
    QString description();

    void setPolicyGroups(QString appName, QStringList groups);
    QStringList policyGroups(QString appName);

    void save() { save(m_fileName); }
    void save(QString fileName);
    void load(QString fileName, QString projectName);
    void reload();

    QString raw();
    void setRaw(QString);

    QString fileName() { return m_fileName; }
    void setFileName(QString fileName) { m_fileName = fileName; emit fileNameChanged(fileName); }

    bool isInitialized() { return m_bInitialized; }
    void nameDashReplaced(){ m_bNameDashReplaced = true; }

protected:
    void callSetFunction(QString functionName, QScriptValueList args);
    void callSetStringListFunction(QString functionName, QStringList args);
    void callSetStringFunction(QString functionName, QString args);
    void callSetFunction(QString functionName, QString arg, QStringList args);

    QScriptValue callGetFunction(QString functionName, QScriptValueList args);
    QStringList callGetStringListFunction(QString functionName);
    QStringList callGetStringListFunction(QString functionName, QString args);
    QString callGetStringFunction(QString functionName);

    QScriptValue callFunction(QString functionName, QScriptValueList args);

    QScriptProgram m_manifestJsApp;
    QScriptEngine engine;

    QString m_userName;

    QString m_fileName;
    QString m_projectName;

    bool m_bInitialized;
    bool m_bNameDashReplaced;
};

}
}

#endif // UBUNTUCLICKMANIFEST_H
