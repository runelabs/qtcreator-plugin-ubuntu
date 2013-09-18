#ifndef UBUNTUPACKAGINGWIDGET_H
#define UBUNTUPACKAGINGWIDGET_H

#include <QWidget>
#include <QProcess>
#include "ubuntubzr.h"
#include "ubuntuclickmanifest.h"

namespace Ui {
class UbuntuPackagingWidget;
}

using namespace Ubuntu::Internal;

class UbuntuPackagingWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit UbuntuPackagingWidget(QWidget *parent = 0);
    ~UbuntuPackagingWidget();

public slots:
    void reload();
    void load_manifest(QString fileName);
    void load_apparmor(QString fileAppArmorName);
    void save(bool bSaveSimple = true);
    void openManifestForProject();
    void setAvailable(bool);
    void load_excludes(QString excludesFile = QLatin1String(""));
    void save_excludes();

protected slots:
    void on_pushButton_addpolicy_clicked();
    void on_pushButtonClickPackage_clicked();
    void on_pushButtonReset_clicked();

    void on_pushButtonReload_clicked();

    void on_tabWidget_currentChanged(int);

    void on_listWidget_customContextMenuRequested(QPoint);
    void bzrChanged();

private:
    UbuntuClickManifest m_manifest;
    UbuntuClickManifest m_apparmor;
    QProcess m_click;
    UbuntuBzr m_bzr;
    QString m_projectName;
    QString m_excludesFile;
    int m_previous_tab;
    Ui::UbuntuPackagingWidget *ui;
};

#endif // UBUNTUPACKAGINGWIDGET_H
