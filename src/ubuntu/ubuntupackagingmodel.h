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

class UbuntuPackagingModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool showValidationUi READ showValidationUi WRITE setShowValidationUi NOTIFY showValidationUiChanged)
    Q_PROPERTY(bool canBuild READ canBuild WRITE setCanBuild NOTIFY canBuildChanged)
    Q_PROPERTY(QAbstractItemModel* validationModel READ validationModel NOTIFY validationModelChanged)
    Q_PROPERTY(QString log READ log WRITE setLog NOTIFY logChanged)

public:

    enum ClickPackageTask {
        None,    //after the project has packaged do nothing
        Verify,  //after the project has packaged verify it
        Install  //after the project has packaged install it on the device
    };

    explicit UbuntuPackagingModel(QObject *parent = 0);
    ~UbuntuPackagingModel();

    bool reviewToolsInstalled ();
    bool showValidationUi() const;
    bool canBuild() const;
    QAbstractItemModel* validationModel() const;
    QString log() const;

public slots:
    void setShowValidationUi(bool arg);
    void setCanBuild(bool arg);
    void buildAndInstallPackageRequested ();
    void buildAndVerifyPackageRequested();
    void on_pushButtonClickPackage_clicked();
    void on_pushButtonReviewersTools_clicked();
    void setLog(QString arg);
    void appendLog(QString arg);

protected slots:
    void onMessage(QString msg);
    void onFinished(QString cmd, int code);
    void onError(QString msg);
    void onStarted(QString);
    void onFinishedAction(const QProcess* proc,QString cmd);

    void checkClickReviewerTool();
    void buildFinished (const bool success);
    void buildPackageRequested();
    void targetChanged();

signals:
    void reviewToolsInstalledChanged(const bool& installed);
    void showValidationUiChanged(bool arg);
    void canBuildChanged(bool arg);
    void validationModelChanged(QAbstractItemModel* arg);
    void logChanged(QString arg);
    void beginValidation();

private:
    void buildClickPackage ();
    void clearPackageBuildList ();
    void updateFrameworkList ();
    void resetValidationResult ();

private:
    bool m_reviewToolsInstalled;
    QMetaObject::Connection m_UbuntuMenu_connection;
    QProcess m_click;
    QString m_projectName;
    QString m_projectDir;
    QString m_reply;
    QString m_reviewesToolsLocation;
    UbuntuProcess m_ubuntuProcess;
    UbuntuValidationResultModel *m_validationModel;
    ClickRunChecksParser* m_inputParser;

    //packaging support with buildsteps
    QList<QSharedPointer<ProjectExplorer::BuildStepList> > m_packageBuildSteps;
    QMetaObject::Connection m_buildManagerConnection;
    ClickPackageTask m_postPackageTask;
    bool m_showValidationUi;
    bool m_canBuild;
    QString m_log;
};


}
}


#endif // UBUNTUPACKAGINGWIDGET_H
