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

#include "ubuntuclicktool.h"

#include <QRegularExpression>
#include <QDir>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMessageBox>
#include <QAction>
#include <QInputDialog>
#include <QPushButton>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/qtcprocess.h>
#include <utils/environment.h>
#include <utils/consoleprocess.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include "ubuntuconstants.h"
#include "ubuntushared.h"

namespace Ubuntu {
namespace Internal {

/**
 * @brief UbuntuClickTool::UbuntuClickTool
 * Implements functionality needed for executing the click
 * tool
 */
UbuntuClickTool::UbuntuClickTool()
{
}

/**
 * @brief UbuntuClickTool::parametersForCreateChroot
 * Initializes a ProjectExplorer::ProcessParameters object with command and arguments
 * to create a new chroot
 */
void UbuntuClickTool::parametersForCreateChroot(const QString &arch, const QString &series, ProjectExplorer::ProcessParameters *params)
{
    QString command = QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_CREATE_ARGS)
            .arg(arch)
            .arg(series);

    params->setCommand(QLatin1String(Constants::UBUNTU_SUDO_BINARY));
    params->setEnvironment(Utils::Environment::systemEnvironment());
    params->setArguments(command);
}

/**
 * @brief UbuntuClickTool::parametersForMaintainChroot
 * Initializes params with the arguments for maintaining the chroot
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForMaintainChroot(const UbuntuClickTool::MaintainMode &mode, const Target &target, ProjectExplorer::ProcessParameters *params)
{
    QString arguments;
    switch (mode) {
    case Upgrade:
        params->setCommand(QLatin1String(Constants::UBUNTU_CLICK_BINARY));
        arguments = QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_UPGRADE_ARGS)
                .arg(target.architecture)
                .arg(target.framework);
        break;
    case Delete:
        params->setCommand(QLatin1String(Constants::UBUNTU_SUDO_BINARY));
        arguments = QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_DESTROY_ARGS)
                .arg(target.architecture)
                .arg(target.framework);
        break;
    }


    params->setEnvironment(Utils::Environment::systemEnvironment());
    params->setArguments(arguments);
}

/**
 * @brief UbuntuClickTool::parametersForCmake
 * Fills ProcessParameters to run cmake inside the Target
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForCmake(const Target &target, const QString &buildDir
                                         ,const QString &relPathToSource, ProjectExplorer::ProcessParameters *params)
{

    QString arguments = QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_CMAKE_ARGS)
            .arg(target.architecture)
            .arg(target.framework)
            .arg(relPathToSource);

    params->setWorkingDirectory(buildDir);
    params->setCommand(QLatin1String(Constants::UBUNTU_CLICK_BINARY));
    params->setArguments(arguments);
    params->setEnvironment(Utils::Environment::systemEnvironment());
}

/**
 * @brief UbuntuClickTool::parametersForMake
 * Fills ProcessParameters to run make inside the Target
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForMake(const UbuntuClickTool::Target &target, const QString &buildDir
                                        , bool doClean, ProjectExplorer::ProcessParameters *params)
{
    QString arguments = QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_MAKE_ARGS)
            .arg(target.architecture)
            .arg(target.framework)
            .arg(doClean ? QLatin1String("clean") : QLatin1String(""));

    params->setWorkingDirectory(buildDir);
    params->setCommand(QLatin1String(Constants::UBUNTU_CLICK_BINARY));
    params->setArguments(arguments);
    params->setEnvironment(Utils::Environment::systemEnvironment());
}

/**
 * @brief UbuntuClickTool::openChrootTerminal
 * Opens a new terminal logged into the chroot specified by \a target
 * The terminal emulator used is specified in the Creator environment option page
 */
void UbuntuClickTool::openChrootTerminal(const UbuntuClickTool::Target &target)
{
    QStringList args = Utils::QtcProcess::splitArgs(Utils::ConsoleProcess::terminalEmulator(Core::ICore::settings()));
    QString     term = args.takeFirst();

    args << QString(QLatin1String(Constants::UBUNTU_CLICK_OPEN_TERMINAL)).arg(target.architecture).arg(target.framework);
    if(!QProcess::startDetached(term,args,QDir::homePath())) {
        printToOutputPane(QLatin1String(Constants::UBUNTU_CLICK_OPEN_TERMINAL_ERROR));
    }
}

/**
 * @brief UbuntuClickTool::listAvailableTargets
 * @return all currently existing chroot targets in the system
 */
QList<UbuntuClickTool::Target> UbuntuClickTool::listAvailableTargets()
{
    QList<Target> items;
    QDir chrootDir(QLatin1String(Constants::UBUNTU_CLICK_CHROOT_BASEPATH));

    //if the dir does not exist there are no available chroots
    if(!chrootDir.exists())
        return items;

    QStringList availableChroots = chrootDir.entryList(QDir::Dirs);

    QRegularExpression clickFilter(QLatin1String(Constants::UBUNTU_CLICK_TARGETS_REGEX));

    //iterate over all chroots and check if they are click chroots
    foreach (const QString &chroot, availableChroots) {
        QRegularExpressionMatch match = clickFilter.match(chroot);
        if(!match.hasMatch()) {
            continue;
        }

        Target t;
        t.framework    = match.captured(1);
        t.architecture = match.captured(2);
        items.append(t);
    }

    return items;
}

/**
 * @brief UbuntuClickTool::targetVersion
 * Reads the ubuntu version from the lsb-release file
 * @returns a QPair containing the major and minor version information
 */
QPair<int, int> UbuntuClickTool::targetVersion(const UbuntuClickTool::Target &target)
{
    QFile f(QString::fromLatin1("%0/click-%1-%2/%3")
            .arg(QLatin1String(Constants::UBUNTU_CLICK_CHROOT_BASEPATH))
            .arg(target.framework)
            .arg(target.architecture)
            .arg(QLatin1String("/etc/lsb-release")));

    if (!f.open(QIODevice::ReadOnly)) {
        //there is no lsb-release file... what now?
        return qMakePair(-1,-1);
    }

    QString info = QString::fromLatin1(f.readAll());

    QRegularExpression grep(QLatin1String(Constants::UBUNTU_CLICK_VERSION_REGEX),QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = grep.match(info);

    if(!match.hasMatch()) {
        return qMakePair(-1,-1);
    }

    bool ok = false;
    int majorV = match.captured(1).toInt(&ok);
    if(!ok) {
        return qMakePair(-1,-1);
    }

    int minorV = match.captured(2).toInt();

    return qMakePair(majorV,minorV);
}



/**
 * @class UbuntuClickManager
 * Build support for click chroot targets, this is a
 * temporary solution until we find a way to make cmakeplugin
 * work like we need it
 */
UbuntuClickManager::UbuntuClickManager(QObject *parent)
    : QObject(parent)
    , m_buildInChrootAction(0)
    , m_failOnError(true)
    , m_process(0)
    , m_futureInterface(0)
    , m_currentBuild(0)
{
    m_process = new Utils::QtcProcess(this);
    connect(m_process,SIGNAL(readyRead()),this,SLOT(on_processReadyRead()));
    connect(m_process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(on_processFinished(int,QProcess::ExitStatus)));
}

void UbuntuClickManager::initialize()
{
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);

    //only visible for a cmake project
    const Core::Context projectContext(CMakeProjectManager::Constants::PROJECTCONTEXT);

    m_buildInChrootAction = new QAction(QIcon(), tr(Constants::UBUNTU_CLICK_BUILD_CONTEXTMENU_TEXT), this);
    Core::Command *command = Core::ActionManager::registerAction(m_buildInChrootAction,
                                                                 Constants::UBUNTU_CLICK_BUILD_CONTEXTMENU_ID, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);

    if(mbuild) {
        mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    }
    connect(m_buildInChrootAction,SIGNAL(triggered()),this,SLOT(on_buildInChrootAction()));
}

void UbuntuClickManager::processBuildQueue()
{
    //build is already running
    if(m_currentBuild)
        return;

    //build queue is empty
    if(m_pendingBuilds.isEmpty()) {
        cleanup();
        return;
    }

    if (!m_futureInterface) {
        m_futureInterface = new QFutureInterface<void>();
        m_futureInterface->setProgressRange(0,5);

        Core::FutureProgress* futureProgress = Core::ICore::progressManager()->addTask(m_futureInterface->future()
                                                                                       ,tr(Constants::UBUNTU_CLICK_BUILDTASK_TITLE)
                                                                                       ,QLatin1String(Constants::UBUNTU_CLICK_BUILDTASK_ID));
        connect(futureProgress,SIGNAL(canceled()),this,SLOT(stop()));
    }

    m_currentBuild = m_pendingBuilds.dequeue();

    ProjectExplorer::BuildConfiguration* bc = m_currentBuild->buildTarget->activeBuildConfiguration();
    if(!bc) {
        delete m_currentBuild;
        m_currentBuild = 0;

        printToOutputPane(tr("--- %0 ---")
                          .arg(QLatin1String(Constants::UBUNTU_CLICK_NOBUILDCONFIG_ERROR))
                          .arg(m_currentBuild->buildTarget->project()->displayName()));

        QMetaObject::invokeMethod(this,"processBuildQueue",Qt::QueuedConnection);
        return;
    }

    //only one build at a time
    setBuildActionsEnabled(false);

    QString buildDirStr = bc->buildDirectory();
    buildDirStr.append(QString::fromLatin1("/%0-%1")
                       .arg(m_currentBuild->targetChroot.framework)
                       .arg(m_currentBuild->targetChroot.architecture));

    QDir buildDir (buildDirStr);
    if(!buildDir.exists())
        buildDir.mkpath(buildDirStr);

    m_currentBuild->buildDir = buildDirStr;

    nextStep();
}

void UbuntuClickManager::stop()
{
    if (m_currentBuild) {
        m_futureInterface->reportCanceled();
        m_futureInterface->reportFinished();

        if (m_process->state() != QProcess::NotRunning) {
            printToOutputPane(tr("--- %0 ---").arg(QLatin1String(Constants::UBUNTU_CLICK_BUILD_CANCELED_MESSAGE)));
            m_process->kill();
            m_process->waitForFinished();
        }
    }

    cleanup();
}

void UbuntuClickManager::cleanup()
{
    if (m_pendingBuilds.size()) {
        qDeleteAll(m_pendingBuilds.begin(),m_pendingBuilds.end());
        m_pendingBuilds.clear();
    }

    if (m_currentBuild) {
        delete m_currentBuild;
        m_currentBuild = 0;
    }

    if (m_futureInterface) {
        delete m_futureInterface;
        m_futureInterface = 0;
    }

    setBuildActionsEnabled(true);
}

/*
 *implementation of the statemachine, runs over
 *all buildsteps needed for a target to be built
 */
void UbuntuClickManager::nextStep()
{
    if(!m_currentBuild)
        return;

    Q_ASSERT(m_process->state() != QProcess::Running);

    switch(m_currentBuild->currentState) {
    case NotStarted: {
        m_currentBuild->currentState = MakeClean;
        ProjectExplorer::ProcessParameters params;

        UbuntuClickTool::parametersForMake(m_currentBuild->targetChroot
                                           ,m_currentBuild->buildDir
                                           ,true //do clean
                                           ,&params);

        params.resolveAll();
        //just continue if make clean fails, maybe we never built before
        m_failOnError = false;

        m_futureInterface->reportStarted();
        m_futureInterface->setProgressValueAndText(1,tr(Constants::UBUNTU_CLICK_BUILD_CLEAN_MESSAGE));
        printToOutputPane(tr(Constants::UBUNTU_CLICK_BUILD_START_MESSAGE).arg(m_currentBuild->buildTarget->project()->displayName()));
        printToOutputPane(tr(Constants::UBUNTU_CLICK_BUILDDIR_MESSAGE).arg(m_currentBuild->buildDir));
        printToOutputPane(tr(Constants::UBUNTU_CLICK_BUILD_CLEAN_MESSAGE));
        startProcess(params);
        break;
    }
    case MakeClean:{
        m_currentBuild->currentState = Cmake;
        ProjectExplorer::ProcessParameters params;

        //get relative directory to source dir
        QDir bd(m_currentBuild->buildDir);
        QString relPath = bd.relativeFilePath(m_currentBuild->buildTarget->project()->projectDirectory());

        UbuntuClickTool::parametersForCmake(m_currentBuild->targetChroot
                                           ,m_currentBuild->buildDir
                                           ,relPath
                                           ,&params);

        params.resolveAll();
        m_failOnError = true;

        m_futureInterface->setProgressValueAndText(2,tr(Constants::UBUNTU_CLICK_BUILD_CMAKE_MESSAGE));
        printToOutputPane(tr(Constants::UBUNTU_CLICK_BUILD_CMAKE_MESSAGE));
        startProcess(params);
        break;
    }
    case Cmake:{
        m_currentBuild->currentState = FixMoc;

        QPair<int,int> chrootVersion = UbuntuClickTool::targetVersion(m_currentBuild->targetChroot);
        if(chrootVersion.first == -1) {
            printToOutputPane(tr(Constants::UBUNTU_CLICK_NOVERSIONINFO_ERROR)
                              .arg(m_currentBuild->targetChroot.framework)
                              .arg(m_currentBuild->targetChroot.architecture));
            stop();
            return;
        }

        if(chrootVersion.first >= 14) { //the fix script needs to run only on targets older than trusty
            printToOutputPane(tr("Building for Ubuntu Version: %0.%1, skipping Automoc Fix").arg(chrootVersion.first).arg(chrootVersion.second));
            nextStep();
            break;
        }

        ProjectExplorer::ProcessParameters params;

        QString arguments = QString::fromLatin1("-c \"%0\"")
                .arg(QLatin1String(Constants::UBUNTU_CLICK_FIXAUTOMOC_SCRIPT));

        params.setWorkingDirectory(m_currentBuild->buildDir);
        params.setCommand(QLatin1String("/bin/bash"));
        params.setArguments(arguments);
        params.setEnvironment(Utils::Environment::systemEnvironment());

        m_futureInterface->setProgressValueAndText(3,tr(Constants::UBUNTU_CLICK_FIXAUTOMOC_MESSAGE));
        printToOutputPane(tr(Constants::UBUNTU_CLICK_FIXAUTOMOC_MESSAGE));
        startProcess(params);
        break;
    }
    case FixMoc:{
        m_currentBuild->currentState = Make;
        ProjectExplorer::ProcessParameters params;

        UbuntuClickTool::parametersForMake(m_currentBuild->targetChroot
                                           ,m_currentBuild->buildDir
                                           ,false //do clean
                                           ,&params);

        params.resolveAll();
        m_failOnError = true;
        m_futureInterface->setProgressValueAndText(4,tr(Constants::UBUNTU_CLICK_MAKE_MESSAGE));
        printToOutputPane(tr(Constants::UBUNTU_CLICK_MAKE_MESSAGE));
        startProcess(params);
        break;
    }
    case Make:{
        m_currentBuild->currentState = Finished;

        delete m_currentBuild;
        m_currentBuild = 0;

        m_futureInterface->reportFinished();
        printToOutputPane(tr("--- %0 ---").arg(QLatin1String(Constants::UBUNTU_CLICK_BUILD_OK_MESSAGE)));

        //give the UI time to show we are finished
        QMetaObject::invokeMethod(this,"processBuildQueue",Qt::QueuedConnection);

        setBuildActionsEnabled(true);
        break;
    }
    default:
        Q_ASSERT(false); //this should never happen
    }
}

/**
 * @brief UbuntuClickManager::startProcess
 * Starts the internal QProcess that executes the current buildstep
 */
void UbuntuClickManager::startProcess(const ProjectExplorer::ProcessParameters &params)
{
    printToOutputPane(tr(Constants::UBUNTU_CLICK_RUN_COMMAND_MESSAGE).arg(params.command()).arg(params.arguments()));
    m_process->setCommand(params.command(),params.arguments());
    m_process->setWorkingDirectory(params.workingDirectory());
    m_process->setEnvironment(params.environment());
    m_process->start();
}

/**
 * @brief UbuntuClickManager::setBuildActionsEnabled
 * Enables or disables the build actions shown in the menu, for example
 * if a build is currently running
 */
void UbuntuClickManager::setBuildActionsEnabled(const bool enabled)
{
    m_buildInChrootAction->setEnabled(enabled);
}

/**
 * @brief UbuntuClickManager::on_buildInChrootAction
 * callback slot that is called by the action
 */
void UbuntuClickManager::on_buildInChrootAction()
{
    ProjectExplorer::Project* currentProject = ProjectExplorer::ProjectExplorerPlugin::currentProject();
    if(!currentProject)
        return;

    ProjectExplorer::Target* buildTarget = currentProject->activeTarget();
    if(!buildTarget)
        return;

    QList<UbuntuClickTool::Target> targets = UbuntuClickTool::listAvailableTargets();
    if (!targets.size()) {
        QMessageBox::warning(Core::ICore::mainWindow()
                             ,tr(Constants::UBUNTU_CLICK_NOTARGETS_TITLE)
                             ,tr(Constants::UBUNTU_CLICK_NOTARGETS_MESSAGE));
        return;
    }

    QStringList items;
    foreach(const UbuntuClickTool::Target& t, targets)
        items << QString::fromLatin1("%0-%1").arg(t.framework).arg(t.architecture);

    bool ok = false;
    QString item = QInputDialog::getItem(Core::ICore::mainWindow()
                                         ,tr(Constants::UBUNTU_CLICK_SELECT_TARGET_TITLE)
                                         ,tr(Constants::UBUNTU_CLICK_SELECT_TARGET_LABEL)
                                         ,items,0,false,&ok);
    //get index of item in the targets list
    int idx = items.indexOf(item);
    if(!ok || idx < 0 || idx >= targets.size())
        return;

    Build* b = new Build();
    b->targetChroot = targets[idx];
    b->buildTarget = buildTarget;
    b->currentState = NotStarted;

    m_pendingBuilds.enqueue(b);
    processBuildQueue();

}

void UbuntuClickManager::on_processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus == QProcess::CrashExit) {
        printToOutputPane(QString::fromLocal8Bit(m_process->readAllStandardError()));

        if(m_failOnError) {
            printToOutputPane(tr("--- %0 ---").arg(QLatin1String(Constants::UBUNTU_CLICK_SELECT_TARGET_LABEL)));
            stop();
        }
    }

    QString errorMsg = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (errorMsg.trimmed().length()>0) printToOutputPane(errorMsg);
    QString msg = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (msg.trimmed().length()>0) printToOutputPane(msg);

    nextStep();
}

void UbuntuClickManager::on_processReadyRead() {
    QString stderr = QString::fromLatin1(m_process->readAllStandardError());
    QString stdout = QString::fromLatin1(m_process->readAllStandardOutput());
    if (!stderr.isEmpty()) {
        printToOutputPane(stderr);
    }
    if (!stdout.isEmpty()) {
        printToOutputPane(stdout);
    }
}

} // namespace Internal
} // namespace Ubuntu

