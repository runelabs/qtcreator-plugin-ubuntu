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
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#ifndef UBUNTUPACKAGINGWIDGET_H
#define UBUNTUPACKAGINGWIDGET_H

#include <QWidget>
#include <QProcess>
#include <QAbstractListModel>
#include <QPointer>
#include "ubuntubzr.h"
#include "ubuntuclickmanifest.h"
#include "ubuntuprocess.h"

#include <projectexplorer/buildstep.h>

namespace Ui {
class UbuntuPackagingWidget;
}

namespace Ubuntu{
namespace Internal{
class UbuntuValidationResultModel;
class ClickRunChecksParser;
}
}

using namespace Ubuntu::Internal;

class UbuntuPackagingWidget : public QWidget
{
    Q_OBJECT
    
public:

    enum ClickPackageTask {
        None,    //after the project has packaged do nothing
        Verify,  //after the project has packaged verify it
        Install  //after the project has packaged install it on the device
    };

    explicit UbuntuPackagingWidget(QWidget *parent = 0);
    ~UbuntuPackagingWidget();

    static QString createPackageName (const QString &userName, const QString &projectName);

    bool reviewToolsInstalled ();
    UbuntuClickManifest *manifest ();
    UbuntuClickManifest *appArmor ();
public slots:
    void autoSave();
    void reload();
    bool load_manifest(QString fileName);
    void load_apparmor(QString fileAppArmorName);
    void save(bool bSaveSimple = true);
    bool openManifestForProject();
    void setAvailable(bool);
    void load_excludes(QString excludesFile = QLatin1String(""));
    void save_excludes();

protected slots:
    void onMessage(QString msg);
    void onFinished(QString cmd, int code);
    void onError(QString msg);
    void onStarted(QString cmd);
    void onFinishedAction(const QProcess* proc,QString cmd);
    void onNewValidationData();
    void onValidationItemSelected(const QModelIndex &index );

    void on_pushButton_addpolicy_clicked();
    void on_pushButtonClickPackage_clicked();
    void on_pushButtonReviewersTools_clicked();
    void onStartupProjectChanged ();
    void onActiveTargetChanged   ();

    void on_pushButtonReload_clicked();

    void on_tabWidget_currentChanged(int);

    void on_listWidget_customContextMenuRequested(QPoint);
    void bzrChanged();

    void checkClickReviewerTool();
    void buildFinished (const bool success);
    void buildAndInstallPackageRequested ();

signals:
    void reviewToolsInstalledChanged(const bool& installed);

protected:
    void addMissingFieldsToManifest(QString fileName);
    void updatePolicyForFramework (const QString &fw);
    void loadManifestDefaults ();
    void loadAppArmorDefaults ();
private slots:
    void on_comboBoxFramework_currentIndexChanged(int index);

private:
    void buildClickPackage ();
    void clearPackageBuildList ();
    void updateFrameworkList ();

private:
    bool m_reviewToolsInstalled;
    UbuntuClickManifest m_manifest;
    UbuntuClickManifest m_apparmor;
    QMetaObject::Connection m_UbuntuMenu_connection;
    QProcess m_click;
    QString m_projectName;
    QString m_projectDir;
    QString m_reply;
    QString m_excludesFile;
    int m_previous_tab;
    QString m_reviewesToolsLocation;
    UbuntuProcess m_ubuntuProcess;
    Ui::UbuntuPackagingWidget *ui;
    UbuntuValidationResultModel *m_validationModel;
    ClickRunChecksParser* m_inputParser;

    //packaging support with buildsteps
    QSharedPointer<ProjectExplorer::BuildStepList> m_packageBuildSteps;
    QMetaObject::Connection m_buildManagerConnection;
    ClickPackageTask m_postPackageTask;

    //current used startup project and target
    QPointer<ProjectExplorer::Project> m_currentProject;
    QPointer<ProjectExplorer::Target>  m_currentTarget;
};
#endif // UBUNTUPACKAGINGWIDGET_H
