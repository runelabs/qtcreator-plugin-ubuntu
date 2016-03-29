/*
 * Copyright 2014 Canonical Ltd.
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

#ifndef UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
#define UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H

#include <QList>
#include <QString>
#include <QDialog>
#include <QFutureInterface>
#include <QQueue>
#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>
#include <QDebug>


class QDialogButtonBox;
class QPlainTextEdit;
class QLabel;
class QAction;
class QTimer;
class QNetworkAccessManager;
class QNetworkReply;

namespace ProjectExplorer {
    class Project;
    class Target;
    class Kit;
}

namespace Ubuntu {
namespace Internal {

class UbuntuClickBuildConfiguration;

class UbuntuClickTool
{
public:

    enum MaintainMode {
        Upgrade,//runs click chroot upgrade
        Delete  //runs click chroot delete
    };

    struct Target {
        QString framework;
        QString architecture;
        QString containerName;
    };

    UbuntuClickTool();

    static void parametersForCreateChroot   (const Target &target, ProjectExplorer::ProcessParameters* params);
    static void parametersForMaintainChroot (const MaintainMode &mode,const Target& target,ProjectExplorer::ProcessParameters* params);

    static void openChrootTerminal (const Target& target);

    static QString targetBasePath (const Target& target);
    static bool getTargetFromUser (Target* target, const QString &framework=QString());
    static QStringList getSupportedFrameworks (const Target *target);
    static QString getMostRecentFramework ( const QString &subFramework, const Target *target );
    static QString findOrCreateGccWrapper(const UbuntuClickTool::Target &target);
    static QString findOrCreateToolWrapper(const QString &tool, const UbuntuClickTool::Target &target);
    static QString findOrCreateQMakeWrapper(const UbuntuClickTool::Target &target);
    static QString findOrCreateMakeWrapper(const UbuntuClickTool::Target &target);
    static QString mapIncludePathsForCMake(ProjectExplorer::Kit *k, const QString &in);
    static QString hostArchitecture ();
    static bool    compatibleWithHostArchitecture (const QString &targetArch);

    static bool          targetExists (const Target& target);
    static QList<Target> listAvailableTargets (const QString &framework=QString());
    static QList<Target> listPossibleDeviceContainers ();
    static const Target *clickTargetFromTarget(ProjectExplorer::Target *t);
    static QString clickChrootSuffix ();
    static QString m_strClickChrootSuffix;
};

QDebug operator<<(QDebug dbg, const UbuntuClickTool::Target& t);

class UbuntuClickFrameworkProvider : public QObject
{
    Q_OBJECT

public:
    UbuntuClickFrameworkProvider();
    static UbuntuClickFrameworkProvider *instance();

    QStringList supportedFrameworks() const;
    QString mostRecentFramework ( const QString &subFramework);
    QString frameworkPolicy (const QString &fw) const;

    static QStringList getSupportedFrameworks ();
    static QString getMostRecentFramework ( const QString &subFramework);
    static QString getBaseFramework (const QString &framework, QStringList *extensions = 0);
    static bool caseInsensitiveFWLessThan(const QString &s1, const QString &s2);
signals:
    void frameworksUpdated();

private slots:
    void requestFinished  ();
    void requestError     ();
    void updateFrameworks (bool force = false);

private:
    void readCache ();
    void readDefaultValues ();
    QStringList parseData  ( const QByteArray &data) const;

private:
    QStringList            m_frameworkCache;
    QString                m_cacheFilePath;
    QMap<QString,QString>  m_policyCache;

    QNetworkAccessManager *m_manager;
    QNetworkReply         *m_currentRequest;
    QTimer                *m_cacheUpdateTimer;
    static UbuntuClickFrameworkProvider *m_instance;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
