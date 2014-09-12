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
#include "ubuntupackagestep.h"
#include "ubuntushared.h"
#include "ubuntucmakecache.h"

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
#include <projectexplorer/toolchain.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <ssh/sshconnection.h>
#include <qmlprojectmanager/qmlprojectconstants.h>


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
    ui->setupUi(this);
    ui->groupBoxValidate->hide();

    m_inputParser = new ClickRunChecksParser(this);
    m_validationModel = new UbuntuValidationResultModel(this);

    connect(m_inputParser,&ClickRunChecksParser::parsedNewTopLevelItem
            ,m_validationModel,&UbuntuValidationResultModel::appendItem);

    ui->treeViewValidate->setModel(m_validationModel);

    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString, int)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));

    connect(ui->treeViewValidate->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),this,SLOT(onValidationItemSelected(QModelIndex)));
    connect(m_validationModel,SIGNAL(rowsInserted(QModelIndex,int,int)),this,SLOT(onNewValidationData()));

    connect(UbuntuMenu::instance(),SIGNAL(requestBuildAndInstallProject()),this,SLOT(buildAndInstallPackageRequested()));
    connect(UbuntuMenu::instance(),SIGNAL(requestBuildAndVerifyProject()),this,SLOT(buildAndVerifyPackageRequested()));
    connect(UbuntuMenu::instance(),SIGNAL(requestBuildProject()),this,SLOT(buildPackageRequested()));

    connect(ui->pushButtonCreateAndInstall,SIGNAL(clicked()),this,SLOT(buildAndInstallPackageRequested()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),SIGNAL(updateRunActions()),this,SLOT(targetChanged()));

    m_reviewToolsInstalled = false;
    checkClickReviewerTool();
}

UbuntuPackagingWidget::~UbuntuPackagingWidget()
{
    delete ui;
    clearPackageBuildList();
}

void UbuntuPackagingWidget::onFinishedAction(const QProcess *proc, QString cmd)
{
    Q_UNUSED(proc);
    Q_UNUSED(cmd);

    disconnect(m_UbuntuMenu_connection);

    if (!m_reviewToolsInstalled)
        return;

    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();

    QVariant manifestPath = UbuntuCMakeCache::getValue(QStringLiteral("UBUNTU_MANIFEST_PATH"),
                                                       startupProject->activeTarget()->activeBuildConfiguration(),
                                                       QStringLiteral("manifest.json"));
    UbuntuClickManifest manifest;
    if(!manifest.load(startupProject->projectDirectory()+QDir::separator()+manifestPath.toString()))
        return;

    QString sClickPackageName;
    QString sClickPackagePath;
    sClickPackageName = QString::fromLatin1("%0_%1_all.click").arg(manifest.name()).arg(manifest.version());
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

void UbuntuPackagingWidget::onMessage(QString msg)
{
    printToOutputPane(msg);
    m_reply.append(msg);
    m_inputParser->addRecievedData(msg);
}

void UbuntuPackagingWidget::onFinished(QString cmd, int code) {
    m_inputParser->endRecieveData();
    if(code == 0)
       m_inputParser->emitTextItem(QLatin1String("Command finished successfully"),cmd,ClickRunChecksParser::NoIcon);
    else
       m_inputParser->emitTextItem(QLatin1String("Command failed"),cmd,ClickRunChecksParser::Error);
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

bool UbuntuPackagingWidget::reviewToolsInstalled()
{
    return m_reviewToolsInstalled;
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

void UbuntuPackagingWidget::on_pushButtonClickPackage_clicked() {
    ProjectExplorer::Project* project = ProjectExplorer::SessionManager::startupProject();
    if(!project)
        return;

    QString mimeType = project->projectManager()->mimeType();
    if(mimeType == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE)
            || mimeType == QLatin1String(Ubuntu::Constants::UBUNTUPROJECT_MIMETYPE)
            || mimeType == QLatin1String(QmlProjectManager::Constants::QMLPROJECT_MIMETYPE)) {
        if(m_reviewToolsInstalled)
            m_postPackageTask = Verify;
        else
            m_postPackageTask = None;
        buildClickPackage();
    } else {
        m_UbuntuMenu_connection =  QObject::connect(UbuntuMenu::instance(),SIGNAL(finished_action(const QProcess*,QString)),this,SLOT(onFinishedAction(const QProcess*,QString)));

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
        UbuntuPackageStep *pckStep = qobject_cast<UbuntuPackageStep*>(m_packageBuildSteps->steps().last());
        if (pckStep && !pckStep->packagePath().isEmpty()) {
            m_ubuntuProcess.stop();

            QString sClickPackagePath = pckStep->packagePath();
            if (sClickPackagePath.isEmpty()) {
                clearPackageBuildList();
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
                    if (!dev->type().toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID)))
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
    clearPackageBuildList();
}

void UbuntuPackagingWidget::buildAndInstallPackageRequested()
{
    m_postPackageTask = Install;
    buildClickPackage();
}

void UbuntuPackagingWidget::buildAndVerifyPackageRequested()
{
    m_postPackageTask = Verify;
    buildClickPackage();
}

void UbuntuPackagingWidget::buildPackageRequested()
{
    m_postPackageTask = None;
    buildClickPackage();
}

void UbuntuPackagingWidget::targetChanged()
{
    ProjectExplorer::Project *p = ProjectExplorer::SessionManager::startupProject();
    bool buildButtonsEnabled = p &&
            p->activeTarget() &&
            p->activeTarget()->kit() &&
            ProjectExplorer::ToolChainKitInformation::toolChain(p->activeTarget()->kit()) &&
            ProjectExplorer::ToolChainKitInformation::toolChain(p->activeTarget()->kit())->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID);

    ui->pushButtonClickPackage->setEnabled(buildButtonsEnabled);
    ui->pushButtonCreateAndInstall->setEnabled(buildButtonsEnabled);
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

    QString mimeType = project->projectManager()->mimeType();
    bool isCMake = mimeType == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE);
    bool isHtml  = mimeType == QLatin1String(Ubuntu::Constants::UBUNTUPROJECT_MIMETYPE);
    bool isQml   = mimeType == QLatin1String(QmlProjectManager::Constants::QMLPROJECT_MIMETYPE);

    if(isCMake || isHtml || isQml) {
        ProjectExplorer::Target* target = project->activeTarget();
        if(!target)
            return;

        ProjectExplorer::Kit* k = target->kit();
        if(!k)
            return;

        if(!ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k).toString().startsWith(QLatin1String(Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID))) {
            QMessageBox::warning(this,tr("Wrong kit type"),tr("It is not supported to create click packages for a non UbuntuSDK target"));
            return;
        }

        if(ProjectExplorer::BuildManager::isBuilding()) {
            QMessageBox::information(this,tr("Build running"),tr("There is currently a build running, please wait for it to be finished"));
            return;
        }

        ProjectExplorer::BuildConfiguration* bc = target->activeBuildConfiguration();
        if(!bc) {
            QMessageBox::information(this,tr("Error"),tr("Please add a valid buildconfiguration to your project"));
            return;
        }

        if(!bc->isEnabled()) {
            QString disabledReason = bc->disabledReason();
            QMessageBox::information(this,tr("Disabled"),tr("The currently selected Buildconfiguration is disabled. %1").arg(disabledReason));
            return;
        }

        clearPackageBuildList();

        m_packageBuildSteps = QSharedPointer<ProjectExplorer::BuildStepList> (new ProjectExplorer::BuildStepList(bc,ProjectExplorer::Constants::BUILDSTEPS_BUILD));
        if (isCMake) {
            //add the normal buildsteps
            m_packageBuildSteps->cloneSteps(bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)));
        }

        //append the click packaging step
        UbuntuPackageStep* package = new UbuntuPackageStep(m_packageBuildSteps.data());
        package->setPackageMode(UbuntuPackageStep::DisableDebugScript);
        m_packageBuildSteps->appendStep(package);

        m_buildManagerConnection = connect(ProjectExplorer::BuildManager::instance(),SIGNAL(buildQueueFinished(bool)),this,SLOT(buildFinished(bool)));

        ProjectExplorer::BuildManager::buildList(m_packageBuildSteps.data(),tr("Build Project"));
    }
}

/*!
 * \brief UbuntuPackagingWidget::clearAdditionalBuildSteps
 * Clears the last used additional buildsteps
 * \note This will cancel a current build if its building the ProjectConfiguration
 *       the BuildSteps belong to!
 */
void UbuntuPackagingWidget::clearPackageBuildList()
{
    if (!m_packageBuildSteps)
        return;

    if(ProjectExplorer::BuildManager::isBuilding( static_cast<ProjectExplorer::ProjectConfiguration *>(m_packageBuildSteps->parent())))
        ProjectExplorer::BuildManager::cancel();

    m_packageBuildSteps->deleteLater();
    m_packageBuildSteps.clear();
}
