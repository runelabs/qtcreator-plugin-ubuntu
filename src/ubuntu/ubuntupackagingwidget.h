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
    explicit UbuntuPackagingWidget(QWidget *parent = 0);
    ~UbuntuPackagingWidget();

    bool reviewToolsInstalled ();
    UbuntuClickManifest *manifest ();
    UbuntuClickManifest *appArmor ();
public slots:
    void autoSave();
    void reload();
    void load_manifest(QString fileName);
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
    void on_pushButtonReset_clicked();
    void on_pushButtonReviewersTools_clicked();

    void on_pushButtonReload_clicked();

    void on_tabWidget_currentChanged(int);

    void on_listWidget_customContextMenuRequested(QPoint);
    void bzrChanged();

    void checkClickReviewerTool();
    void buildFinished (const bool success);

signals:
    void reviewToolsInstalledChanged(const bool& installed);

private slots:
    void on_comboBoxFramework_currentIndexChanged(int index);

private:
    void clearAdditionalBuildSteps ();

private:
    bool m_reviewToolsInstalled;
    UbuntuClickManifest m_manifest;
    UbuntuClickManifest m_apparmor;
    QMetaObject::Connection m_UbuntuMenu_connection;
    QProcess m_click;
    UbuntuBzr m_bzr;
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
    QList<QPointer<ProjectExplorer::BuildStep> > m_additionalPackagingBuildSteps;
    QMetaObject::Connection m_buildManagerConnection;
};
#endif // UBUNTUPACKAGINGWIDGET_H
