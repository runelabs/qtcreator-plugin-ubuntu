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

#include "ubuntupackagingmodel.h"
#include "ubuntuconstants.h"
#include "ubuntumenu.h"
#include "ubuntuclicktool.h"
#include "ubuntucmakemakestep.h"
#include "ubuntuvalidationresultmodel.h"
#include "ubuntudevice.h"
#include "ubuntupackagestep.h"
#include "ubuntushared.h"
#include "ubuntucmakecache.h"
#include "ubuntuprojecthelper.h"
#include "ubuntufixmanifeststep.h"
#include "wizards/ubuntufatpackagingwizard.h"
#include "clicktoolchain.h"

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
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QListWidgetItem>

#include <QMenu>
#include <QMessageBox>
#include <QScriptEngine>
#include <QRegularExpression>
#include <QApplication>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuPackagingModel::UbuntuPackagingModel(QObject *parent) :
    QObject(parent),
    m_postPackageTask(Verify)
{
    setShowValidationUi(false);

    m_inputParser = new ClickRunChecksParser(this);
    m_validationModel = new UbuntuValidationResultModel(this);

    connect(m_inputParser,&ClickRunChecksParser::parsedNewTopLevelItem
            ,m_validationModel,&UbuntuValidationResultModel::appendItem);
    connect(m_inputParser,SIGNAL(begin()),this,SIGNAL(beginValidation()));

    connect(&m_ubuntuProcess,SIGNAL(started(QString)),this,SLOT(onStarted(QString)));
    connect(&m_ubuntuProcess,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_ubuntuProcess,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString, int)));
    connect(&m_ubuntuProcess,SIGNAL(error(QString)),this,SLOT(onError(QString)));

    connect(UbuntuMenu::instance(),SIGNAL(requestBuildAndInstallProject()),this,SLOT(buildAndInstallPackageRequested()));
    connect(UbuntuMenu::instance(),SIGNAL(requestBuildAndVerifyProject()),this,SLOT(buildAndVerifyPackageRequested()));
    connect(UbuntuMenu::instance(),SIGNAL(requestBuildProject()),this,SLOT(buildPackageRequested()));

    //connect(ui->pushButtonCreateAndInstall,SIGNAL(clicked()),this,SLOT(buildAndInstallPackageRequested()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),SIGNAL(updateRunActions()),this,SLOT(targetChanged()));

    m_reviewToolsInstalled = false;
    checkClickReviewerTool();
}

UbuntuPackagingModel::~UbuntuPackagingModel()
{
    clearPackageBuildList();
}

void UbuntuPackagingModel::onFinishedAction(const QProcess *proc, QString cmd)
{
    Q_UNUSED(proc);
    Q_UNUSED(cmd);

    disconnect(m_UbuntuMenu_connection);

    if (!m_reviewToolsInstalled)
        return;

    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();

    //first try to use the version in the deploy directory. It is always in the root of the click package
    QString manifestPath;

    if(startupProject->activeTarget() && startupProject->activeTarget()->activeBuildConfiguration()) {
        manifestPath = startupProject->activeTarget()->activeBuildConfiguration()->buildDirectory()
                .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                .appendPath(QStringLiteral("manifest.json"))
                .toString();
    }

    if(!QFile::exists(manifestPath)) {
        //fall back to the project directory
        manifestPath = UbuntuProjectHelper::getManifestPath(startupProject->activeTarget(),
                                                            Utils::FileName::fromString(startupProject->projectDirectory())
                                                            .appendPath(QStringLiteral("manifest.json")).toString());
    }

    UbuntuClickManifest manifest;
    if(!manifest.load(manifestPath))
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

        m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::CLICK_REVIEWERSTOOLS_LOCATION).arg(sClickPackagePath));
        m_ubuntuProcess.start(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE)).arg(sClickPackagePath));
    }

}

void UbuntuPackagingModel::onMessage(QString msg)
{
    appendLog(msg);
    printToOutputPane(msg);
    m_reply.append(msg);
    m_inputParser->addRecievedData(msg);
}

void UbuntuPackagingModel::onFinished(QString cmd, int code) {
    m_inputParser->endRecieveData();

    if(code == 0)
       appendLog(tr("Command finished successfully\n"));
    else
       appendLog(tr("Command failed\n"));

    if (cmd == QString::fromLatin1(Constants::UBUNTUWIDGETS_ONFINISHED_SCRIPT_LOCAL_PACKAGE_INSTALLED).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)) {
        resetValidationResult();
        QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
        foreach(QString line, lines) {
            line = line.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
                // There is no click reviewer tool installed
                m_reviewToolsInstalled = false;
                setShowValidationUi(false);
                emit reviewToolsInstalledChanged(m_reviewToolsInstalled);
            } else {
                QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                QString sReviewerToolPackageStatus = lineData.takeFirst();
                //QString sReviewerToolPackageName = lineData.takeFirst();
                //QString sReviewerToolPackageVersion = lineData.takeFirst();
                if (sReviewerToolPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
                    // There is click reviewer tool installed
                    setShowValidationUi(true);
                    m_reviewToolsInstalled = true;
                    emit reviewToolsInstalledChanged(m_reviewToolsInstalled);
                }
            }
        }
    }
    m_reply.clear();
}

void UbuntuPackagingModel::onError(QString msg) {
    if(msg.isEmpty())
        return;

    appendLog(tr("Error: %1\n").arg(msg));
}

void UbuntuPackagingModel::onStarted(QString ) {
    resetValidationResult();
    setLog(tr("Start Command\n"));
}

bool UbuntuPackagingModel::reviewToolsInstalled()
{
    return m_reviewToolsInstalled;
}

bool UbuntuPackagingModel::showValidationUi() const
{
    return m_showValidationUi;
}

bool UbuntuPackagingModel::canBuild() const
{
    return m_canBuild;
}

QAbstractItemModel *UbuntuPackagingModel::validationModel() const
{
    return m_validationModel;
}

QString UbuntuPackagingModel::log() const
{
    return m_log;
}

void UbuntuPackagingModel::setShowValidationUi(bool arg)
{
    if (m_showValidationUi != arg) {
        m_showValidationUi = arg;
        emit showValidationUiChanged(arg);
    }
}

void UbuntuPackagingModel::setCanBuild(bool arg)
{
    if (m_canBuild != arg) {
        m_canBuild = arg;
        emit canBuildChanged(arg);
    }
}

void UbuntuPackagingModel::on_pushButtonReviewersTools_clicked() {
    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();
    m_ubuntuProcess.stop();

    QString directory = QDir::homePath();
    if(startupProject) directory = startupProject->projectDirectory();

    QString clickPackage = QFileDialog::getOpenFileName(Core::ICore::mainWindow(),QString(QLatin1String(Constants::UBUNTU_CLICK_PACKAGE_SELECTOR_TEXT)),QString(QLatin1String("%0/..")).arg(directory),QLatin1String(Constants::UBUNTU_CLICK_PACKAGE_MASK));
    if (clickPackage.isEmpty()) return;
    m_inputParser->beginRecieveData();
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::CLICK_REVIEWERSTOOLS_LOCATION).arg(clickPackage));
    m_ubuntuProcess.start(QString(QLatin1String(Constants::UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE)).arg(clickPackage));
}

void UbuntuPackagingModel::setLog(QString arg)
{
    if (m_log != arg) {
        m_log = arg;
        emit logChanged(arg);
    }
}

void UbuntuPackagingModel::appendLog(QString arg)
{
    m_log.append(arg);
    emit logChanged(m_log);
}

void UbuntuPackagingModel::on_pushButtonClickPackage_clicked() {
    ProjectExplorer::Project* project = ProjectExplorer::SessionManager::startupProject();
    if(!project)
        return;

    QString mimeType = project->projectManager()->mimeType();
    if(mimeType == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE)
            || mimeType == QLatin1String(Ubuntu::Constants::UBUNTUPROJECT_MIMETYPE)
            || mimeType == QLatin1String(QmlProjectManager::Constants::QMLPROJECT_MIMETYPE)
            || mimeType == QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE)) {
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

void UbuntuPackagingModel::checkClickReviewerTool() {
    m_ubuntuProcess.stop();
    QString sReviewerPackageName = QLatin1String(Ubuntu::Constants::REVIEWER_PACKAGE_NAME);
    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sReviewerPackageName) << QApplication::applicationDirPath());
    m_ubuntuProcess.start(QString::fromLatin1(Constants::UBUNTUPACKAGINGWIDGET_LOCAL_REVIEWER_INSTALLED));
}

void UbuntuPackagingModel::buildFinished(const bool success)
{
    disconnect(m_buildManagerConnection);
    if (success) {
        //the step that created the click package is always in the last list and always the last step
        UbuntuPackageStep *pckStep = qobject_cast<UbuntuPackageStep*>(m_packageBuildSteps.last()->steps().last());
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

                    m_inputParser->beginRecieveData();
                    m_ubuntuProcess.append(QStringList() << QString::fromLatin1(Constants::CLICK_REVIEWERSTOOLS_LOCATION).arg(sClickPackagePath));
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

void UbuntuPackagingModel::buildAndInstallPackageRequested()
{
    m_postPackageTask = Install;
    buildClickPackage();
}

void UbuntuPackagingModel::buildAndVerifyPackageRequested()
{
    m_postPackageTask = Verify;
    buildClickPackage();
}

void UbuntuPackagingModel::buildPackageRequested()
{
    m_postPackageTask = None;
    buildClickPackage();
}

void UbuntuPackagingModel::targetChanged()
{
    ProjectExplorer::Project *p = ProjectExplorer::SessionManager::startupProject();
    bool buildButtonsEnabled = p &&
            p->activeTarget() &&
            p->activeTarget()->kit() &&
            ProjectExplorer::ToolChainKitInformation::toolChain(p->activeTarget()->kit()) &&
            (ProjectExplorer::ToolChainKitInformation::toolChain(p->activeTarget()->kit())->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
             ||  p->projectManager()->mimeType() == QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE));

    setCanBuild(buildButtonsEnabled);
}


/*!
 * \brief UbuntuPackagingWidget::buildClickPackage
 * Starts the build of a cmake project. Make sure to set
 * m_postPackageTask correctly before calling this function
 */
void UbuntuPackagingModel::buildClickPackage()
{
    ProjectExplorer::Project* project = ProjectExplorer::SessionManager::startupProject();
    if(!project) {
        QMessageBox::warning(Core::ICore::mainWindow(),tr("No Project"),tr("No valid project loaded."));
        return;
    }

    QString mimeType = project->projectManager()->mimeType();
    bool isCMake = mimeType == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE);
    bool isHtml  = mimeType == QLatin1String(Ubuntu::Constants::UBUNTUPROJECT_MIMETYPE);
    bool isQml   = mimeType == QLatin1String(QmlProjectManager::Constants::QMLPROJECT_MIMETYPE);
    bool isQmake = mimeType == QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE);

    if (isQmake) {
        QmakeProjectManager::QmakeProject *qmakePro = static_cast<QmakeProjectManager::QmakeProject *>(project);
        UbuntuFatPackagingWizard wiz(qmakePro);
        if (wiz.exec() != QDialog::Accepted)
            return;

        int mode = wiz.field(QStringLiteral("mode")).toInt();
        QString workingDir = wiz.field(QStringLiteral("targetDirectory")).toString();
        QString deployDir  = QStringLiteral("%1/deploy").arg(workingDir);
        QList<ProjectExplorer::BuildConfiguration *> suspects;

        if(wiz.field(QStringLiteral("mode")).toInt() == 1)  {
            suspects = wiz.selectedTargets();
        } else {
            if(project->activeTarget()) {
                suspects.append(project->activeTarget()->activeBuildConfiguration());
            }
        }

        if (!suspects.isEmpty()) {

            QStringList usedArchitectures;
            clearPackageBuildList();
            //@TODO check if different frameworks have been used
            bool firstStep=true;
            foreach (ProjectExplorer::BuildConfiguration *b, suspects) {
                m_packageBuildSteps.append(QSharedPointer<ProjectExplorer::BuildStepList> (new ProjectExplorer::BuildStepList(b,ProjectExplorer::Constants::BUILDSTEPS_BUILD)));

                ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(b->target()->kit());
                if(tc && tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)){
                    ClickToolChain *cTc = static_cast<ClickToolChain *>(tc);
                    usedArchitectures << cTc->clickTarget().architecture;
                }

                //add the normal buildsteps
                m_packageBuildSteps.last()->cloneSteps(b->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)));

                //append the make install step
                UbuntuPackageStep* package = new UbuntuPackageStep(m_packageBuildSteps.last().data());
                package->setDebugMode(UbuntuPackageStep::DisableDebugScript);
                package->setOverrideDeployDir(deployDir);
                package->setPackageMode(UbuntuPackageStep::OnlyMakeInstall);
                package->setReferenceBuildConfig(b);
                package->setCleanDeployDirectory(firstStep);
                m_packageBuildSteps.last()->appendStep(package);

                firstStep=false;
            }

            UbuntuFixManifestStep *fixManifest = new UbuntuFixManifestStep(m_packageBuildSteps.last().data());
            if (mode == 0)
                fixManifest->setArchitectures(QStringList()<<QStringLiteral("all"));
            else
                fixManifest->setArchitectures(usedArchitectures);
            fixManifest->setPackageDir(deployDir);
            m_packageBuildSteps.last()->appendStep(fixManifest);

            //append the click package step
            UbuntuPackageStep* package = new UbuntuPackageStep(m_packageBuildSteps.last().data());
            package->setDebugMode(UbuntuPackageStep::DisableDebugScript);
            package->setOverrideDeployDir(deployDir);
            package->setOverrideClickWorkingDir(workingDir);
            package->setPackageMode(UbuntuPackageStep::OnlyClickBuild);
            m_packageBuildSteps.last()->appendStep(package);

            m_buildManagerConnection = connect(ProjectExplorer::BuildManager::instance(),SIGNAL(buildQueueFinished(bool)),this,SLOT(buildFinished(bool)));

            QList<ProjectExplorer::BuildStepList *> rawSteps;
            QStringList rawStepMessages;
            foreach (QSharedPointer<ProjectExplorer::BuildStepList> l, m_packageBuildSteps) {
                rawSteps << l.data();
                rawStepMessages << tr("Build %1").arg(l->target()->displayName());
            }

            ProjectExplorer::BuildManager::buildLists(rawSteps,rawStepMessages);
        }

    } else if(isCMake || isHtml || isQml) {
        ProjectExplorer::Target* target = project->activeTarget();
        if(!target)
            return;

        ProjectExplorer::Kit* k = target->kit();
        if(!k)
            return;

        if(!ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k).toString().startsWith(QLatin1String(Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID))) {
            QMessageBox::warning(Core::ICore::mainWindow(),tr("Wrong kit type"),tr("It is not supported to create click packages for a non UbuntuSDK target"));
            return;
        }

        if(ProjectExplorer::BuildManager::isBuilding()) {
            QMessageBox::information(Core::ICore::mainWindow(),tr("Build running"),tr("There is currently a build running, please wait for it to be finished"));
            return;
        }

        ProjectExplorer::BuildConfiguration* bc = target->activeBuildConfiguration();
        if(!bc) {
            QMessageBox::information(Core::ICore::mainWindow(),tr("Error"),tr("Please add a valid buildconfiguration to your project"));
            return;
        }

        if(!bc->isEnabled()) {
            QString disabledReason = bc->disabledReason();
            QMessageBox::information(Core::ICore::mainWindow(),tr("Disabled"),tr("The currently selected Buildconfiguration is disabled. %1").arg(disabledReason));
            return;
        }

        clearPackageBuildList();

        m_packageBuildSteps.append(QSharedPointer<ProjectExplorer::BuildStepList> (new ProjectExplorer::BuildStepList(bc,ProjectExplorer::Constants::BUILDSTEPS_BUILD)));
        if (isCMake || isQmake) {
            //add the normal buildsteps
            m_packageBuildSteps.last()->cloneSteps(bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)));
        }

        //append the click packaging step
        UbuntuPackageStep* package = new UbuntuPackageStep(m_packageBuildSteps.last().data());
        package->setDebugMode(UbuntuPackageStep::DisableDebugScript);
        m_packageBuildSteps.last()->appendStep(package);

        m_buildManagerConnection = connect(ProjectExplorer::BuildManager::instance(),SIGNAL(buildQueueFinished(bool)),this,SLOT(buildFinished(bool)));

        ProjectExplorer::BuildManager::buildList(m_packageBuildSteps.last().data(),tr("Build Project"));
    }
}

/*!
 * \brief UbuntuPackagingWidget::clearAdditionalBuildSteps
 * Clears the last used additional buildsteps
 * \note This will cancel a current build if its building the ProjectConfiguration
 *       the BuildSteps belong to!
 */
void UbuntuPackagingModel::clearPackageBuildList()
{
    if (m_packageBuildSteps.isEmpty())
        return;

    foreach(QSharedPointer<ProjectExplorer::BuildStepList> list, m_packageBuildSteps) {
        if(ProjectExplorer::BuildManager::isBuilding( static_cast<ProjectExplorer::ProjectConfiguration *>(list->parent())))
            ProjectExplorer::BuildManager::cancel();

        list->deleteLater();
        list.clear();
    }

    m_packageBuildSteps.clear();
}

void UbuntuPackagingModel::resetValidationResult()
{
    m_validationModel->clear();
}

}}
