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

#include "ubuntupackagingwidget.h"
#include "ubuntusecuritypolicypickerdialog.h"
#include "ui_ubuntupackagingwidget.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <QFileDialog>
#include <QJsonDocument>
#include <QListWidgetItem>

#include <QMenu>
#include <QMessageBox>
#include <QScriptEngine>

using namespace Ubuntu::Internal;

UbuntuPackagingWidget::UbuntuPackagingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuPackagingWidget)
{
    m_previous_tab = 0;
    ui->setupUi(this);

    ui->tabWidget->setCurrentIndex(0);
    ui->stackedWidget->setCurrentIndex(1);
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(&m_bzr,SIGNAL(initializedChanged()),SLOT(bzrChanged()));
    connect(&m_manifest,SIGNAL(loaded()),SLOT(reload()));
    connect(&m_apparmor,SIGNAL(loaded()),SLOT(reload()));

    m_bzr.initialize();
}

void UbuntuPackagingWidget::setAvailable(bool available) {
    if (available) {
        ui->stackedWidget_2->setCurrentWidget(ui->pageAvailable);
    } else {
        ui->stackedWidget_2->setCurrentWidget(ui->pageNotAvailable);
    }
}

UbuntuPackagingWidget::~UbuntuPackagingWidget()
{
    save((ui->tabWidget->currentWidget() == ui->tabSimple));

    delete ui;
}

void UbuntuPackagingWidget::openManifestForProject() {

    ProjectExplorer::ProjectExplorerPlugin* projectExplorerInstance = ProjectExplorer::ProjectExplorerPlugin::instance();
    //ProjectExplorer::SessionManager* sessionManager = projectExplorerInstance->session();
    ProjectExplorer::Project* startupProject = projectExplorerInstance->startupProject();

    if (startupProject) {
        m_projectName = startupProject->displayName();

        QString fileName = QString(QLatin1String("%0/manifest.json")).arg(startupProject->projectDirectory());
        QString no_underscore_displayName = startupProject->displayName();

        m_projectName=no_underscore_displayName;
        no_underscore_displayName.replace(QLatin1String("_"),QLatin1String("-"));
        if (no_underscore_displayName != startupProject->displayName()) {
            m_manifest.nameDashReplaced();

        }
        QString fileAppArmorName = QString(QLatin1String("%0/%1.json")).arg(startupProject->projectDirectory()).arg(no_underscore_displayName);
        if (QFile(fileName).exists()==false) {
            m_manifest.setFileName(fileName);
            on_pushButtonReset_clicked();
        } else {
            load_manifest(fileName);
        }
        if (QFile(fileAppArmorName).exists()==false) {
            m_apparmor.setFileName(fileAppArmorName);
            on_pushButtonReset_clicked();
        } else {
            load_apparmor(fileAppArmorName);
        }

        load_excludes(QString(QLatin1String("%0/.excludes")).arg(startupProject->projectDirectory()));

        ui->stackedWidget->setCurrentIndex(0);
    } else {
        m_projectName.clear();
        ui->stackedWidget->setCurrentIndex(1);
    }
}

void UbuntuPackagingWidget::bzrChanged() {
    // left empty on purpose
    m_manifest.setMaintainer(m_bzr.whoami());
    QString userName = m_bzr.launchpadId();
    if (userName.isEmpty()) userName = QLatin1String("username");
    // Commented out for bug #1219948  - https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1219948
    //m_manifest.setName(QString(QLatin1String("com.ubuntu.developer.%0.%1")).arg(userName).arg(m_projectName));
    reload();
}

void UbuntuPackagingWidget::on_pushButtonReset_clicked() {
    QString fileName = m_manifest.fileName();
    QString fileAppArmorName = m_apparmor.fileName();
    load_manifest(QLatin1String(":/ubuntu/manifest.json.template"));
    load_apparmor(QLatin1String(":/ubuntu/myapp.json.template"));
    m_apparmor.setFileName(fileAppArmorName);
    m_manifest.setFileName(fileName);
    m_manifest.setMaintainer(m_bzr.whoami());
    QString userName = m_bzr.launchpadId();
    if (userName.isEmpty()) userName = QLatin1String("username");
    m_manifest.setName(QString(QLatin1String("com.ubuntu.developer.%0.%1")).arg(userName).arg(m_projectName));
    reload();
}

void UbuntuPackagingWidget::save(bool bSaveSimple) {
    switch (m_previous_tab) {
    case 0: {
        // set package name to lower, bug #1219877
        m_manifest.setName(ui->lineEdit_name->text().toLower());
        m_manifest.setMaintainer(ui->lineEdit_maintainer->text());
        m_manifest.setVersion(ui->lineEdit_version->text());
        m_manifest.setTitle(ui->lineEdit_title->text());
        m_manifest.setDescription(ui->lineEdit_description->text());
        QStringList items;
        for (int i=0; i<ui->listWidget->count(); i++) {
            // Fix bug #1221407 - make sure that there are no empty policy groups.
            QString policyGroup = ui->listWidget->item(i)->text().trimmed();
            if (!policyGroup.isEmpty()) {
                items.append(ui->listWidget->item(i)->text());
            }
        }
        m_apparmor.setPolicyGroups(m_projectName,items);
        m_manifest.save();
        break;
    }
    case 1: {
        m_manifest.setRaw(ui->plainTextEditJson->toPlainText());
        m_manifest.save();
        break;
    }
    case 2: {
        m_apparmor.setRaw(ui->plainTextEditAppArmorJson->toPlainText());
        m_apparmor.save();
        break;
    }
    case 3: {
        save_excludes();
        break;
    }
    }
}

void UbuntuPackagingWidget::load_manifest(QString fileName) {
    m_manifest.load(fileName,m_projectName);
    m_manifest.setMaintainer(m_bzr.whoami());
    QString userName = m_bzr.launchpadId();
    if (userName.isEmpty()) userName = QLatin1String("username");
    m_projectName.replace(QLatin1String("_"),QLatin1String("-"));

    // Commented out for bug #1219948  - https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1219948
    //m_manifest.setName(QString(QLatin1String("com.ubuntu.developer.%0.%1")).arg(userName).arg(m_projectName));
}

void UbuntuPackagingWidget::load_apparmor(QString fileAppArmorName) {
    m_apparmor.load(fileAppArmorName,m_projectName);
}

void UbuntuPackagingWidget::load_excludes(QString excludesFile) {
    if (!excludesFile.isEmpty()) m_excludesFile = excludesFile;

    QFile f(m_excludesFile);
    if (f.exists()) {
        if (f.open(QIODevice::ReadOnly)) {
            ui->plainTextEdit_excludes->setPlainText(QString::fromAscii(f.readAll()));
            f.close();
        }
    }
}

void UbuntuPackagingWidget::save_excludes() {
    QFile f(m_excludesFile);

    if (f.open(QIODevice::WriteOnly)) {
        f.write(ui->plainTextEdit_excludes->toPlainText().toAscii());
        f.close();
    }

}

void UbuntuPackagingWidget::reload() {
    ui->lineEdit_maintainer->setText(m_manifest.maintainer());
    ui->lineEdit_name->setText(m_manifest.name());
    ui->lineEdit_title->setText(m_manifest.title());
    ui->lineEdit_version->setText(m_manifest.version());
    ui->lineEdit_description->setText(m_manifest.description());

    QStringList policyGroups = m_apparmor.policyGroups(m_projectName);

    ui->listWidget->clear();
    foreach( QString policy, policyGroups) {
        QListWidgetItem* item = new QListWidgetItem(policy);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->listWidget->addItem(item);
    }

    ui->plainTextEditJson->setPlainText(m_manifest.raw());
    ui->plainTextEditAppArmorJson->setPlainText(m_apparmor.raw());

    load_excludes(QLatin1String(""));
}

void UbuntuPackagingWidget::on_tabWidget_currentChanged(int tab) {
    save((ui->tabWidget->currentWidget() != ui->tabSimple));

    m_previous_tab = tab;
    reload();
}

void UbuntuPackagingWidget::on_pushButtonReload_clicked() {
    m_manifest.reload();
    m_apparmor.reload();
}

void UbuntuPackagingWidget::on_listWidget_customContextMenuRequested(QPoint p) {
    if (ui->listWidget->selectedItems().count()==0) { return; }

    QMenu contextMenu;
    contextMenu.addAction(QLatin1String("Remove"));
    QAction* selectedItem = contextMenu.exec(ui->listWidget->mapToGlobal(p));
    if (selectedItem) {
        delete ui->listWidget->currentItem();
    }
}

void UbuntuPackagingWidget::on_pushButton_addpolicy_clicked() {
    UbuntuSecurityPolicyPickerDialog dialog;
    if (dialog.exec()) {
        ui->listWidget->addItems(dialog.selectedPolicyGroups());
    }
    /*QListWidgetItem* item = new QListWidgetItem(ui->lineEdit_policy->text());
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->listWidget->addItem(item);
    ui->lineEdit_policy->clear();*/
}

void UbuntuPackagingWidget::on_pushButtonClickPackage_clicked() {

    save((ui->tabWidget->currentWidget() == ui->tabSimple));

    Core::Command *cmd = Core::ActionManager::instance()->command(Core::Id("Ubuntu.Build.Package"));
    if (cmd) {
        cmd->action()->trigger();
    }
}
