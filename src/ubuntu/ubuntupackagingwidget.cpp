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
#include "ubuntumenu.h"
#include "ubuntuclicktool.h"

using namespace Ubuntu;

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <projectexplorer/target.h>

#include <QFileDialog>
#include <QJsonDocument>
#include <QListWidgetItem>

#include <QMenu>
#include <QMessageBox>
#include <QScriptEngine>
#include <QRegularExpression>
#include "ubuntuvalidationresultmodel.h"

using namespace Ubuntu::Internal;

UbuntuPackagingWidget::UbuntuPackagingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuPackagingWidget)
{
    m_previous_tab = 0;

    ui->setupUi(this);
    ui->groupBoxPackaging->hide();
    ui->groupBoxValidate->hide();

    ui->tabWidget->setCurrentIndex(0);
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    m_inputParser = new ClickRunChecksParser(this);
    m_validationModel = new UbuntuValidationResultModel(this);

    connect(m_inputParser,&ClickRunChecksParser::parsedNewTopLevelItem
            ,m_validationModel,&UbuntuValidationResultModel::appendItem);

    ui->treeViewValidate->setModel(m_validationModel);

    connect(&m_bzr,SIGNAL(initializedChanged()),SLOT(bzrChanged()));
    connect(&m_manifest,SIGNAL(loaded()),SLOT(reload()));
    connect(&m_apparmor,SIGNAL(loaded()),SLOT(reload()));
    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString, int)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));

    connect(ui->treeViewValidate->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),this,SLOT(onValidationItemSelected(QModelIndex)));
    connect(m_validationModel,SIGNAL(rowsInserted(QModelIndex,int,int)),this,SLOT(onNewValidationData()));

    m_bzr.initialize();

    ui->comboBoxFramework->blockSignals(true);
    ui->comboBoxFramework->addItems(UbuntuClickTool::getSupportedFrameworks());
    ui->comboBoxFramework->blockSignals(false);

    m_reviewToolsInstalled = false;
    checkClickReviewerTool();
}

void UbuntuPackagingWidget::onFinishedAction(const QProcess *proc, QString cmd) {

    const bool isCMakeCmd = (cmd == QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_ONFINISHED_ACTION_CLICK_CMAKECREATE).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) && (ui->pushButtonReviewersTools->isVisible());
    const bool isQmlCmd   = (cmd == QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_ONFINISHED_ACTION_CLICK_CREATE).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) && (ui->pushButtonReviewersTools->isVisible());

    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();

    QString sClickPackageName;
    QString sClickPackagePath;

    if(isCMakeCmd) {
        QStringList args = proc->arguments();
        QString arch      = args[0];
        QString framework = args[1];
        sClickPackageName = QString::fromLatin1("%0_%1_%2.click").arg(ui->lineEdit_name->text()).arg(ui->lineEdit_version->text()).arg(arch);
        sClickPackagePath = QString::fromLatin1("%0/%1-%2/")
                .arg(startupProject->activeTarget()->activeBuildConfiguration()->buildDirectory().toString())
                .arg(framework)
                .arg(arch);
    }else {
        sClickPackageName = QString::fromLatin1("%0_%1_all.click").arg(ui->lineEdit_name->text()).arg(ui->lineEdit_version->text());
        sClickPackagePath = startupProject->projectDirectory();

        QRegularExpression re(QLatin1String("\\/\\w+$")); // search for the project name in the path
        QRegularExpressionMatch match = re.match(sClickPackagePath);
        if (match.hasMatch()) {
            QString matched = match.captured(0);
            sClickPackagePath.chop(matched.length()-1); //leave the slash
        }
    }

    sClickPackagePath.append(sClickPackageName);
    m_ubuntuProcess.stop();
    if (sClickPackagePath.isEmpty()) {
        disconnect(m_UbuntuMenu_connection);
        return;
    }


    qDebug()<<"Going to verify: "<<sClickPackagePath;

    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION).arg(sClickPackagePath));
    m_ubuntuProcess.start(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE)).arg(sClickPackagePath));

    disconnect(m_UbuntuMenu_connection);
}

void UbuntuPackagingWidget::onNewValidationData()
{
    if(!ui->treeViewValidate->selectionModel()->hasSelection()) {
        QModelIndex index = m_validationModel->findFirstErrorItem();

        ui->treeViewValidate->setCurrentIndex(index);
    }
}

void UbuntuPackagingWidget::onValidationItemSelected(const QModelIndex &index)
{
    if(index.isValid()) {
        QUrl link = m_validationModel->data(index,UbuntuValidationResultModel::LinkRole).toUrl();
        if(link.isValid()) {
            ui->labelErrorLink->setText(
                        QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_LINK_DISPLAYTEXT)
                        .arg(link.toString(QUrl::FullyEncoded)));
        } else {
            ui->labelErrorLink->setText(QLatin1String(""));
        }
        ui->labelErrorType->setText(m_validationModel->data(index,UbuntuValidationResultModel::TypeRole).toString());
        ui->plainTextEditDescription->setPlainText(m_validationModel->data(index,UbuntuValidationResultModel::DescriptionRole).toString());
    }
}

void UbuntuPackagingWidget::onMessage(QString msg) {
    m_reply.append(msg);
    m_inputParser->addRecievedData(msg);
}

void UbuntuPackagingWidget::onFinished(QString cmd, int code) {
    Q_UNUSED(code);
    m_inputParser->endRecieveData();
    if (cmd == QString::fromLatin1(Constants::UBUNTUWIDGETS_ONFINISHED_SCRIPT_LOCAL_PACKAGE_INSTALLED).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        m_validationModel->clear();
        QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
        QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY), QLatin1String(Constants::SETTINGS_PRODUCT));
        settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_CLICK));
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
                // There is no click reviewer tool installed
                m_reviewToolsInstalled = false;
                ui->groupBoxValidate->hide();
                settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS_LOCATION), QLatin1String(Constants::EMPTY));
                settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS), Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS);

                emit reviewToolsInstalledChanged(m_reviewToolsInstalled);
            } else {
                QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                QString sReviewerToolPackageStatus = lineData.takeFirst();
                QString sReviewerToolPackageName = lineData.takeFirst();
                QString sReviewerToolPackageVersion = lineData.takeFirst();
                if (sReviewerToolPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
                    // There is click reviewer tool installed
                    ui->groupBoxValidate->show();
                    m_reviewToolsInstalled = true;
                    settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS_LOCATION), QLatin1String(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION));
                    settings.setValue(QLatin1String(Constants::SETTINGS_KEY_CLICK_REVIEWERSTOOLS), Constants::SETTINGS_CLICK_REVIEWERSTOOLS_TRUE);

                    emit reviewToolsInstalledChanged(m_reviewToolsInstalled);
                }
            }
        }
    }
    m_reply.clear();
}

void UbuntuPackagingWidget::onError(QString msg) {
    if(msg.isEmpty())
        return;

    m_inputParser->emitTextItem(QLatin1String("Error"),msg,ClickRunChecksParser::Error);
}

void UbuntuPackagingWidget::onStarted(QString cmd) {
    m_validationModel->clear();
    m_inputParser->emitTextItem(QLatin1String("Start Command"),cmd,ClickRunChecksParser::NoIcon);
}


void UbuntuPackagingWidget::setAvailable(bool available) {
    if (available) {
        ui->groupBoxPackaging->setVisible(true);
    } else {
        ui->groupBoxPackaging->setVisible(false);
    }
}

UbuntuPackagingWidget::~UbuntuPackagingWidget()
{
    autoSave();
    delete ui;
}

bool UbuntuPackagingWidget::reviewToolsInstalled()
{
    return m_reviewToolsInstalled;
}

UbuntuClickManifest *UbuntuPackagingWidget::manifest()
{
    return &m_manifest;
}

UbuntuClickManifest *UbuntuPackagingWidget::appArmor()
{
    return &m_apparmor;
}

void UbuntuPackagingWidget::on_pushButtonReviewersTools_clicked() {
    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();
    m_ubuntuProcess.stop();

    QString directory = QDir::homePath();
    if(startupProject) directory = startupProject->projectDirectory();

    QString clickPackage = QFileDialog::getOpenFileName(this,QString(QLatin1String(Constants::UBUNTU_CLICK_PACKAGE_SELECTOR_TEXT)),QString(QLatin1String("%0/..")).arg(directory),QLatin1String(Constants::UBUNTU_CLICK_PACKAGE_MASK));
    if (clickPackage.isEmpty()) return;
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION).arg(clickPackage));
    m_ubuntuProcess.start(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE)).arg(clickPackage));
}

void UbuntuPackagingWidget::autoSave() {
    save((ui->tabWidget->currentWidget() == ui->tabSimple));
}

bool UbuntuPackagingWidget::openManifestForProject() {

    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();

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
        return true;
    } else {
        m_projectName.clear();
    }
    return false;
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
    Q_UNUSED(bSaveSimple);
    switch (m_previous_tab) {
    case 0: {
        // set package name to lower, bug #1219877
        m_manifest.setName(ui->lineEdit_name->text().toLower());
        m_manifest.setMaintainer(ui->lineEdit_maintainer->text());
        m_manifest.setVersion(ui->lineEdit_version->text());
        m_manifest.setTitle(ui->lineEdit_title->text());
        m_manifest.setDescription(ui->lineEdit_description->text());

        if(ui->comboBoxFramework->currentText() != tr(Constants::UBUNTU_UNKNOWN_FRAMEWORK_NAME))
            m_manifest.setFrameworkName(ui->comboBoxFramework->currentText());

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
    // Commented out for bug #1274265 https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1274265
    //m_manifest.setMaintainer(m_bzr.whoami());
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

    int idx = ui->comboBoxFramework->findText(m_manifest.frameworkName());

    //disable the currentIndexChanged signal, reloading the files
    //should never change the contents of the files (e.g. policy_version)
    ui->comboBoxFramework->blockSignals(true);
    //if the framework name is not valid set to empty item

    //just some data to easily find the unknown framework item without
    //using string compare
    if(idx < 0) {
        if(ui->comboBoxFramework->findData(Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA) < 0)
            ui->comboBoxFramework->addItem(tr(Constants::UBUNTU_UNKNOWN_FRAMEWORK_NAME),Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA);

        ui->comboBoxFramework->setCurrentIndex(ui->comboBoxFramework->count()-1);
    } else {
        ui->comboBoxFramework->setCurrentIndex(idx);
        ui->comboBoxFramework->removeItem(ui->comboBoxFramework->findData(Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA));
    }

    ui->comboBoxFramework->blockSignals(false);

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
    m_UbuntuMenu_connection =  QObject::connect(UbuntuMenu::instance(),SIGNAL(finished_action(const QProcess*,QString)),this,SLOT(onFinishedAction(const QProcess*,QString)));
    save((ui->tabWidget->currentWidget() == ui->tabSimple));

    if(!m_UbuntuMenu_connection)
        qDebug()<<"Could not connect signals";

    QAction* action = 0;
    if(ProjectExplorer::SessionManager::startupProject()->projectManager()->mimeType()
            == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE)) {
        action = UbuntuMenu::menuAction(Core::Id(Constants::UBUNTUPACKAGINGWIDGET_BUILDCMAKEPACKAGE_ID));
    } else {
        action = UbuntuMenu::menuAction(Core::Id(Constants::UBUNTUPACKAGINGWIDGET_BUILDPACKAGE_ID));
    }

    if(action) {
        action->trigger();
    }
}

void UbuntuPackagingWidget::checkClickReviewerTool() {
    m_ubuntuProcess.stop();
    QString sReviewerPackageName = QLatin1String(Ubuntu::Constants::REVIEWER_PACKAGE_NAME);
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sReviewerPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_LOCAL_REVIEWER_INSTALLED));
}

void UbuntuPackagingWidget::on_comboBoxFramework_currentIndexChanged(int index)
{
    if(ui->comboBoxFramework->itemText(index).startsWith(QLatin1String(Constants::UBUNTU_FRAMEWORK_14_04_BASENAME))) {
        ui->comboBoxFramework->removeItem(ui->comboBoxFramework->findData(Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA));
        m_apparmor.setPolicyVersion(QLatin1String("1.1"));
    } else if(ui->comboBoxFramework->itemText(index).startsWith(QLatin1String(Constants::UBUNTU_FRAMEWORK_13_10_BASENAME))) {
        ui->comboBoxFramework->removeItem(ui->comboBoxFramework->findData(Constants::UBUNTU_UNKNOWN_FRAMEWORK_DATA));
        m_apparmor.setPolicyVersion(QLatin1String("1.0"));
    } else {
        return;
    }

    m_apparmor.save();
}
