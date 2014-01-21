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
#include "ubuntuconstants.h"

using namespace Ubuntu;

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
    ui->pushButtonReviewersTools->hide();

    ui->tabWidget->setCurrentIndex(0);
    ui->stackedWidget->setCurrentIndex(1);
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(&m_bzr,SIGNAL(initializedChanged()),SLOT(bzrChanged()));
    connect(&m_manifest,SIGNAL(loaded()),SLOT(reload()));
    connect(&m_apparmor,SIGNAL(loaded()),SLOT(reload()));
    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString, int)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));
    m_bzr.initialize();
    checkClickReviewerTool();
}

void UbuntuPackagingWidget::on_pushButtonClosePackageReviewTools_clicked() {
    ui->stackedWidget->setCurrentIndex(0);
}

void UbuntuPackagingWidget::onMessage(QString msg) {
    m_reply.append(msg);

    ui->plainTextEditPackageReview->appendPlainText(msg);
}

void UbuntuPackagingWidget::onFinished(QString cmd, int code) {
    ui->plainTextEditPackageReview->appendPlainText(QLatin1String("*** DONE ***"));
    if (cmd == QString::fromLatin1(Constants::UBUNTUWIDGETS_ONFINISHED_SCRIPT_LOCAL_PACKAGE_INSTALLED).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
	QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
        settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_CLICK));
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
		// There is no click reviewer tool installed
                ui->pushButtonReviewersTools->hide();
                settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS_LOCATION), QLatin1String(Constants::EMPTY));
                settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS), Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS);
            } else {
                QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                QString sEmulatorPackageStatus = lineData.takeFirst();
                QString sEmulatorPackageName = lineData.takeFirst();
                QString sEmulatorPackageVersion = lineData.takeFirst();
                if (sEmulatorPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
			// There is click reviewer tool installed
			ui->pushButtonReviewersTools->show();
			settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS_LOCATION), QLatin1String(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION));
			settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS), Constants::SETTINGS_CLICK_REVIEWERSTOOLS_TRUE);
                }
            }
        }
    }
    m_reply.clear();
}

void UbuntuPackagingWidget::onError(QString msg) {
    ui->plainTextEditPackageReview->appendPlainText(msg);
}

void UbuntuPackagingWidget::onStarted(QString cmd) {
    ui->plainTextEditPackageReview->clear();
    ui->plainTextEditPackageReview->appendPlainText(cmd);
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
    autoSave();
    delete ui;
}

void UbuntuPackagingWidget::on_pushButtonReviewersTools_clicked() {
    m_ubuntuProcess.stop();
    ProjectExplorer::ProjectExplorerPlugin* projectExplorerInstance = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project* startupProject = projectExplorerInstance->startupProject();
    QString clickPackage = QFileDialog::getOpenFileName(this,QString(QLatin1String(Constants::UBUNTU_CLICK_PACKAGE_SELECTOR_TEXT)),QString(QLatin1String("%0/..")).arg(startupProject->projectDirectory()),QLatin1String(Constants::UBUNTU_CLICK_PACKAGE_MASK));
    if (clickPackage.isEmpty()) return;
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION).arg(clickPackage));
    ui->stackedWidget->setCurrentIndex(2);
    m_ubuntuProcess.start(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE)).arg(clickPackage));
}

void UbuntuPackagingWidget::autoSave() {
    save((ui->tabWidget->currentWidget() == ui->tabSimple));
}

void UbuntuPackagingWidget::openManifestForProject() {

    ProjectExplorer::ProjectExplorerPlugin* projectExplorerInstance = ProjectExplorer::ProjectExplorerPlugin::instance();
    ///ProjectExplorer::SessionManager* sessionManager = projectExplorerInstance->session();
    ProjectExplorer::Project* startupProject = projectExplorerInstance->startupProject();

    if (startupProject) {
        m_projectName = startupProject->displayName();

        QString fileName = QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_OPENMANIFEST)).arg(startupProject->projectDirectory());
        QString no_underscore_displayName = startupProject->displayName();

        m_projectName=no_underscore_displayName;
        no_underscore_displayName.replace(QLatin1String(Constants::UNDERLINE),QLatin1String(Constants::DASH));
        if (no_underscore_displayName != startupProject->displayName()) {
            m_manifest.nameDashReplaced();

        }
        QString fileAppArmorName = QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_APPARMOR)).arg(startupProject->projectDirectory()).arg(no_underscore_displayName);
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

        load_excludes(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_EXCLUDES)).arg(startupProject->projectDirectory()));

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
    if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
    // Commented out for bug #1219948  - https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1219948
    //m_manifest.setName(QString(QLatin1String("com.ubuntu.developer.%0.%1")).arg(userName).arg(m_projectName));
    reload();
}

void UbuntuPackagingWidget::on_pushButtonReset_clicked() {
    QString fileName = m_manifest.fileName();
    QString fileAppArmorName = m_apparmor.fileName();
    load_manifest(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MANIFEST));
    load_apparmor(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MYAPP));
    m_apparmor.setFileName(fileAppArmorName);
    m_manifest.setFileName(fileName);
    m_manifest.setMaintainer(m_bzr.whoami());
    QString userName = m_bzr.launchpadId();
    if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
    m_manifest.setName(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_NAME)).arg(userName).arg(m_projectName));
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
        m_apparmor.save();
        save_excludes();

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
    if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
    m_projectName.replace(QLatin1String(Constants::UNDERLINE),QLatin1String(Constants::DASH));

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

    load_excludes(QLatin1String(Constants::EMPTY));
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
    contextMenu.addAction(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_MENU_REMOVE));
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
}

void UbuntuPackagingWidget::on_pushButtonClickPackage_clicked() {

    save((ui->tabWidget->currentWidget() == ui->tabSimple));

    Core::Command *cmd = Core::ActionManager::instance()->command(Core::Id(Constants::UBUNTUPACKAGINGWIDGET_BUILDPACKAGE_ID));
    if (cmd) {
        cmd->action()->trigger();
    }
}

void UbuntuPackagingWidget::checkClickReviewerTool() {
    m_ubuntuProcess.stop();
    QString sReviewerPackageName = QLatin1String(Ubuntu::Constants::REVIEWER_PACKAGE_NAME);
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sReviewerPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_LOCAL_REVIEWER_INSTALLED));
}

