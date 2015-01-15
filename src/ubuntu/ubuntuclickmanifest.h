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
#include <QScriptValue>

namespace ProjectExplorer { class Project; }

namespace Ubuntu {
namespace Internal {

class UbuntuClickManifest : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuClickManifest(QObject *parent = 0);

    struct Hook {
        enum Type{
            Invalid,
            Scope,
            Application,
            Helper
        };

        QString appId;
        QString desktopFile;
        QString scope;
        QString appArmorFile;

        Type type() const;
    };

signals:
    void nameChanged();
    void maintainerChanged();
    void titleChanged();
    void policyGroupsChanged();
    void policyVersionChanged();
    void versionChanged();
    void descriptionChanged();
    void saved();
    void loaded();
    void error();
    void fileNameChanged(QString);
    void frameworkNameChanged(const QString &name);
    void appArmorFileNameChanged (const QString &appId, const QString &name);

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

    void setPolicyGroups(QStringList groups);
    QStringList policyGroups();

    QList<Hook> hooks ();
    void setHook (const Hook &hook);
    
    void setPolicyVersion(const QString &version);
    QString policyVersion();

    void setFrameworkName (const QString& name);
    QString frameworkName ();

    QString appArmorFileName ( const QString &appId );
    bool setAppArmorFileName( const QString &appId, const QString &name );

    bool enableDebugging();

    void save() { save(m_fileName); }
    void save(QString fileName);
    bool load(const QString &fileName, ProjectExplorer::Project *proj = 0, QString *errorMessage = 0);
    bool loadFromString(const QString &data);
    void reload();

    QString raw();
    void setRaw(QString);

    QString fileName() { return m_fileName; }
    void setFileName(QString fileName) { m_fileName = fileName; emit fileNameChanged(fileName); }

    bool isInitialized() const { return m_bInitialized; }
    void nameDashReplaced(){ m_bNameDashReplaced = true; }

protected:
    void callSetFunction(QString functionName, QScriptValueList args);
    void callSetStringListFunction(QString functionName, QStringList args);
    void callSetStringFunction(QString functionName, QString args);

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
