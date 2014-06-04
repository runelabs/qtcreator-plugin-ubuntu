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
#include "ubuntucmakemakestep.h"
#include "ubuntuvalidationresultmodel.h"
#include "ubuntudevice.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/buildsteplist.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <ssh/sshconnection.h>


#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QListWidgetItem>

#include <QMenu>
#include <QMessageBox>
#include <QScriptEngine>
#include <QRegularExpression>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

enum {
    debug = 0
};

UbuntuPackagingWidget::UbuntuPackagingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UbuntuPackagingWidget),
    m_postPackageTask(Verify)
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

    connect(UbuntuMenu::instance(),SIGNAL(requestBuildAndInstallProject()),this,SLOT(buildAndInstallPackageRequested()));

    m_bzr.initialize();

    ui->comboBoxFramework->blockSignals(true);
    ui->comboBoxFramework->addItems(UbuntuClickTool::getSupportedFrameworks());
    ui->comboBoxFramework->blockSignals(false);

    m_reviewToolsInstalled = false;
    checkClickReviewerTool();
}

UbuntuPackagingWidget::~UbuntuPackagingWidget()
{
    autoSave();
    delete ui;

    clearAdditionalBuildSteps();
}

QString UbuntuPackagingWidget::createPackageName(const QString &userName, const QString &projectName)
{
    return QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_NAME)).arg(userName).arg(projectName);
}

void UbuntuPackagingWidget::onFinishedAction(const QProcess *proc, QString cmd)
{
    Q_UNUSED(proc);
    Q_UNUSED(cmd);

    disconnect(m_UbuntuMenu_connection);

    if (!m_reviewToolsInstalled)
        return;

    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();

    QString sClickPackageName;
    QString sClickPackagePath;
    sClickPackageName = QString::fromLatin1("%0_%1_all.click").arg(ui->lineEdit_name->text()).arg(ui->lineEdit_version->text());
    sClickPackagePath = startupProject->projectDirectory();

    QRegularExpression re(QLatin1String("\\/\\w+$")); // search for the project name in the path
    QRegularExpressionMatch match = re.match(sClickPackagePath);
    if (match.hasMatch()) {
        QString matched = match.captured(0);
        sClickPackagePath.chop(matched.length()-1); //leave the slash
    }

    sClickPackagePath.append(sClickPackageName);
    m_ubuntuProcess.stop();

    if (sClickPackagePath.isEmpty())
        return;

    if (m_reviewToolsInstalled) {

        if(debug) qDebug()<<"Going to verify: "<<sClickPackagePath;

        m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION).arg(sClickPackagePath));
        m_ubuntuProcess.start(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE)).arg(sClickPackagePath));
    }

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
        m_projectDir  = startupProject->projectDirectory();

        QString fileName = QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_OPENMANIFEST)).arg(startupProject->projectDirectory());
        QString no_underscore_displayName = startupProject->displayName();

        m_projectName=no_underscore_displayName;
        no_underscore_displayName.replace(QLatin1String(Constants::UNDERLINE),QLatin1String(Constants::DASH));
        if (no_underscore_displayName != startupProject->displayName()) {
            m_manifest.nameDashReplaced();

        }

        QString defaultAppArmorName = QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_APPARMOR))
                .arg(startupProject->projectDirectory())
                .arg(no_underscore_displayName);

        if (QFile(fileName).exists()==false) {
            m_manifest.setFileName(fileName);
            m_apparmor.setFileName(defaultAppArmorName);
            on_pushButtonReset_clicked();
        } else {
            if(!load_manifest(fileName))
                return false;
        }

        //we just support the first hook for now
        //@TODO proper support for multiple hooks
        QList<UbuntuClickManifest::Hook> hooks = m_manifest.hooks();

        QString fileAppArmorName;
        if(hooks.isEmpty())
            fileAppArmorName = defaultAppArmorName;
        else
            fileAppArmorName = QString(QLatin1String("%1/%2")).arg(startupProject->projectDirectory()).arg(hooks[0].appArmorFile);

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
        m_projectDir.clear();
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
    m_manifest.setFileName(fileName);

    QDir projectDir(m_projectDir);
    if(fileAppArmorName.isEmpty() || !QFile::exists(fileAppArmorName)) {
        fileAppArmorName = m_manifest.appArmorFileName(m_manifest.hooks()[0].appId);
        fileAppArmorName = projectDir.absoluteFilePath(fileAppArmorName);
    } else {
        m_manifest.setAppArmorFileName(m_manifest.hooks()[0].appId,projectDir.relativeFilePath(fileAppArmorName));
    }

    m_apparmor.setFileName(fileAppArmorName);
    m_apparmor.save();

    m_manifest.setMaintainer(m_bzr.whoami());
    QString userName = m_bzr.launchpadId();
    if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
    m_manifest.setName(createPackageName(userName,m_projectName));
    m_manifest.save();
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

            QList<UbuntuClickManifest::Hook> hooks = m_manifest.hooks();
            if(hooks.size()) {
                QString appArmorFileName = hooks[0].appArmorFile;
                appArmorFileName = QString(QLatin1String("%1/%2"))
                        .arg(m_projectDir)
                        .arg(appArmorFileName);
                if (appArmorFileName != m_apparmor.fileName()) {
                    if(!QFile::exists(appArmorFileName)) {
                        load_apparmor(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MYAPP));
                        m_apparmor.setFileName(appArmorFileName);
                    } else {
                        load_apparmor(appArmorFileName);
                    }
                }
            }

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

void UbuntuPackagingWidget::addMissingFieldsToManifest (QString fileName)
{
    QFile in(fileName);
    if(!in.open(QIODevice::ReadOnly))
        return;

    QFile templateFile(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_DEFAULT_MANIFEST));
    if(!templateFile.open(QIODevice::ReadOnly))
        return;

    QJsonParseError err;
    QJsonDocument templateDoc = QJsonDocument::fromJson(templateFile.readAll(),&err);
    if(err.error != QJsonParseError::NoError)
        return;

    QJsonDocument inDoc = QJsonDocument::fromJson(in.readAll(),&err);
    if(err.error != QJsonParseError::NoError)
        return;

    in.close();

    QJsonObject templateObject = templateDoc.object();
    QJsonObject targetObject = inDoc.object();
    QJsonObject::const_iterator i = templateObject.constBegin();

    bool changed = false;
    for(;i != templateObject.constEnd(); i++) {
        if(!targetObject.contains(i.key())) {
            changed = true;

            if(debug) qDebug()<<"Manifest file missing key: "<<i.key();

            if (i.key() == QStringLiteral("name")) {
                QString userName = m_bzr.launchpadId();
                if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
                targetObject.insert(i.key(),createPackageName(userName,m_projectName));

                if(debug) qDebug()<<"Setting to "<<createPackageName(userName,m_projectName);
            } else if (i.key() == QStringLiteral("maintainer")) {
                targetObject.insert(i.key(),m_bzr.whoami());

                if(debug) qDebug()<<"Setting to "<<m_bzr.whoami();
            } else if (i.key() == QStringLiteral("framework")) {
                targetObject.insert(i.key(),UbuntuClickTool::getMostRecentFramework( QString() ));
            } else {
                targetObject.insert(i.key(),i.value());

                if(debug) qDebug() <<"Setting to "<<i.value();
            }
        }
    }

    if (changed) {
        if(!in.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return;
        QJsonDocument doc(targetObject);
        QByteArray data = doc.toJson();
        in.write(data);
        in.close();
    }
}

bool UbuntuPackagingWidget::load_manifest(QString fileName) {

    addMissingFieldsToManifest(fileName);

   if(! m_manifest.load(fileName,m_projectName) )
       return false;
    // Commented out for bug #1274265 https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1274265
    //m_manifest.setMaintainer(m_bzr.whoami());
    QString userName = m_bzr.launchpadId();
    if (userName.isEmpty()) userName = QLatin1String(Constants::USERNAME);
    m_projectName.replace(QLatin1String(Constants::UNDERLINE),QLatin1String(Constants::DASH));
    // Commented out for bug #1219948  - https://bugs.launchpad.net/qtcreator-plugin-ubuntu/+bug/1219948
    //m_manifest.setName(QString(QLatin1String("com.ubuntu.developer.%0.%1")).arg(userName).arg(m_projectName));
    return true;
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
    UbuntuSecurityPolicyPickerDialog dialog(m_apparmor.policyVersion());
    if (dialog.exec()) {
        ui->listWidget->addItems(dialog.selectedPolicyGroups());
    }
}

void UbuntuPackagingWidget::on_pushButtonClickPackage_clicked() {
    ProjectExplorer::Project* project = ProjectExplorer::SessionManager::startupProject();
    if(!project)
        return;

    if(project->projectManager()->mimeType() == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE)) {
        if(m_reviewToolsInstalled)
            m_postPackageTask = Verify;
        else
            m_postPackageTask = None;
        buildClickPackage();
    } else {
        m_UbuntuMenu_connection =  QObject::connect(UbuntuMenu::instance(),SIGNAL(finished_action(const QProcess*,QString)),this,SLOT(onFinishedAction(const QProcess*,QString)));
        save((ui->tabWidget->currentWidget() == ui->tabSimple));

        if(!m_UbuntuMenu_connection)
            qWarning()<<"Could not connect signals";

        QAction* action = UbuntuMenu::menuAction(Core::Id(Constants::UBUNTUPACKAGINGWIDGET_BUILDPACKAGE_ID));
        if(action) {
            action->trigger();
        }
    }
}

void UbuntuPackagingWidget::checkClickReviewerTool() {
    m_ubuntuProcess.stop();
    QString sReviewerPackageName = QLatin1String(Ubuntu::Constants::REVIEWER_PACKAGE_NAME);
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sReviewerPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_LOCAL_REVIEWER_INSTALLED));
}

void UbuntuPackagingWidget::buildFinished(const bool success)
{
    disconnect(m_buildManagerConnection);
    if (success) {
        UbuntuClickPackageStep *pckStep = qobject_cast<UbuntuClickPackageStep*>(m_additionalPackagingBuildSteps.last());
        if (pckStep && !pckStep->packagePath().isEmpty()) {
            m_ubuntuProcess.stop();

            QString sClickPackagePath = pckStep->packagePath();
            if (sClickPackagePath.isEmpty()) {
                clearAdditionalBuildSteps();
                return;
            }

            switch (m_postPackageTask) {
                case None:
                    break;
                case Verify: {
                    if(debug) qDebug()<<"Going to verify: "<<sClickPackagePath;

                    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::SETTINGS_DEFAULT_CLICK_REVIEWERSTOOLS_LOCATION).arg(sClickPackagePath));
                    m_ubuntuProcess.start(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE)).arg(sClickPackagePath));
                    break;
                }
                case Install: {
                    ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(pckStep->target()->kit());
                    if (!dev)
                        break; //fall through to clear buildsteps
                    if (dev->type() != Constants::UBUNTU_DEVICE_TYPE_ID)
                        break; //fall through to clear buildsteps

                    UbuntuDevice::ConstPtr ubuntuDev = qSharedPointerCast<const UbuntuDevice>(dev);
                    QSsh::SshConnectionParameters connParams = ubuntuDev->sshParameters();

                    m_ubuntuProcess.append(QStringList()
                                           << QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_CLICK_DEPLOY_SCRIPT)
                                           .arg(Constants::UBUNTU_SCRIPTPATH)
                                           .arg(ubuntuDev->serialNumber())
                                           .arg(sClickPackagePath)
                                           .arg(QStringLiteral("%1@%2").arg(connParams.userName).arg(connParams.host))
                                           .arg(connParams.port)
                                           .arg(QStringLiteral("/home/%1/dev_tmp").arg(connParams.userName))
                                           .arg(connParams.userName));
                    m_ubuntuProcess.start(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_DEPLOY_MESSAGE));
                    break;
                }
            }
        }
    }
    clearAdditionalBuildSteps();
}

void UbuntuPackagingWidget::buildAndInstallPackageRequested()
{
    m_postPackageTask = Install;
    buildClickPackage();
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

/*!
 * \brief UbuntuPackagingWidget::buildClickPackage
 * Starts the build of a cmake project. Make sure to set
 * m_postPackageTask correctly before calling this function
 */
void UbuntuPackagingWidget::buildClickPackage()
{
    ProjectExplorer::Project* project = ProjectExplorer::SessionManager::startupProject();
    if(!project)
        return;

    if(project->projectManager()->mimeType() == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE)) {
        ProjectExplorer::Target* target = project->activeTarget();
        if(!target)
            return;

        ProjectExplorer::Kit* k = target->kit();
        if(!k)
            return;

        if(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k) != Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID) {
            QMessageBox::warning(this,tr("Wrong kit type"),tr("It is not supported to create click packages for a non UbuntuSDK target"));
            return;
        }

        ProjectExplorer::BuildConfiguration* bc = target->activeBuildConfiguration();
        if(!bc)
            return;

        if(!bc->isEnabled()) {
            QString disabledReason = bc->disabledReason();
            QMessageBox::information(this,tr("Disabled"),tr("The currently selected Buildconfiguration is disabled. %1").arg(disabledReason));
            return;
        }

        if(ProjectExplorer::BuildManager::isBuilding()) {
            QMessageBox::information(this,tr("Build running"),tr("There is currently a build running, please wait for it to be finished"));
            return;
        }

        clearAdditionalBuildSteps();

        ProjectExplorer::BuildStepList* steps = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
        if(!steps || steps->isEmpty())
            return;

        UbuntuCMakeDeployStep* deplStep = new UbuntuCMakeDeployStep(steps);
        m_additionalPackagingBuildSteps.append(deplStep);

        UbuntuClickPackageStep* package = new UbuntuClickPackageStep(steps);
        m_additionalPackagingBuildSteps.append(package);

        m_buildManagerConnection = connect(ProjectExplorer::BuildManager::instance(),SIGNAL(buildQueueFinished(bool)),this,SLOT(buildFinished(bool)));

        ProjectExplorer::BuildManager::buildList(steps,tr("Build Project"));
        ProjectExplorer::BuildManager::appendStep(deplStep,tr("Preparing Click package"));
        ProjectExplorer::BuildManager::appendStep(package ,tr("Creating Click package"));

    }
}

/*!
 * \brief UbuntuPackagingWidget::clearAdditionalBuildSteps
 * Clears the last used additional buildsteps
 * \note This will cancel a current build if its building the ProjectConfiguration
 *       the BuildSteps belong to!
 */
void UbuntuPackagingWidget::clearAdditionalBuildSteps()
{
    foreach(QPointer<ProjectExplorer::BuildStep> step,m_additionalPackagingBuildSteps) {
        if(step) {
            if(ProjectExplorer::BuildManager::isBuilding(step->projectConfiguration()))
                ProjectExplorer::BuildManager::cancel();
            delete step.data();
        }
    }
    m_additionalPackagingBuildSteps.clear();
}
