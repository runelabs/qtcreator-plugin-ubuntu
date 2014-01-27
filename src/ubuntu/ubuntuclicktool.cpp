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
#include <QDebug>
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
#include <texteditor/fontsettings.h>

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
    params->setCommand(QLatin1String("pkexec"));

    QStringList clickArgs;
    clickArgs << QLatin1String("click")
              << QLatin1String("chroot")
              << QLatin1String("-a")
              << arch
              << QLatin1String("-s")
              << series
              << QLatin1String("create");

    QStringList sudoArgs;
    sudoArgs << QLatin1String("sh")
             << QLatin1String("-c")
             << Utils::QtcProcess::joinArgs(clickArgs);

    params->setEnvironment(Utils::Environment::systemEnvironment());
    params->setArguments(Utils::QtcProcess::joinArgs(sudoArgs));
}

/**
 * @brief UbuntuClickTool::parametersForMaintainChroot
 * Initializes params with the arguments for maintaining the chroot
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForMaintainChroot(const UbuntuClickTool::MaintainMode &mode, const Target &target, ProjectExplorer::ProcessParameters *params)
{
    params->setCommand(QLatin1String("click"));

    QString strMode;
    switch (mode) {
    case Upgrade:
        strMode = QLatin1String("upgrade");
        break;
    case Delete:
        strMode = QLatin1String("destroy");
        break;
    }

    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << strMode;

    params->setEnvironment(Utils::Environment::systemEnvironment());
    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
}

/**
 * @brief UbuntuClickTool::parametersForCmake
 * Fills ProcessParameters to run cmake inside the Target
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForCmake(const Target &target, const QString &buildDir
                                         ,const QString &relPathToSource, ProjectExplorer::ProcessParameters *params)
{
    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << QLatin1String("run")
              << QLatin1String("cmake")
              << QLatin1String("-DCMAKE_TOOLCHAIN_FILE=/etc/dpkg-cross/cmake/CMakeCross.txt")
              << relPathToSource;

    params->setWorkingDirectory(buildDir);
    params->setCommand(QLatin1String("click"));
    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
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
    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << QLatin1String("run")
              << QLatin1String("make");

    if(doClean)
        arguments << QLatin1String("clean");

    params->setWorkingDirectory(buildDir);
    params->setCommand(QLatin1String("click"));
    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
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

    qDebug()<<"Going to use terminal emulator: "<<term;

    args << QString(QLatin1String("click chroot -a %0 -f %1 maint /bin/bash")).arg(target.architecture).arg(target.framework);
    if(!QProcess::startDetached(term,args,QDir::homePath())) {
        qDebug()<<"Error when starting terminal";
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

    QRegularExpression clickFilter(QLatin1String("^click-(.*)-([A-Za-z0-9]+)$"));

    //iterate over all chroots and check if they are click chroots
    foreach (const QString &chroot, availableChroots) {
        QRegularExpressionMatch match = clickFilter.match(chroot);
        if(!match.hasMatch()) {
            qDebug()<<"Skipping: "<<chroot;
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

    QRegularExpression grep(QLatin1String("^DISTRIB_RELEASE=([0-9]+)\\.([0-9]+)$"),QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = grep.match(info);

    if(!match.hasMatch()) {
        qDebug()<<"No version information found....";
        return qMakePair(-1,-1);
    }

    bool ok = false;
    int majorV = match.captured(1).toInt(&ok);
    if(!ok) {
        qDebug()<<"Failed to convert: "<<match.captured(0);
        return qMakePair(-1,-1);
    }

    int minorV = match.captured(2).toInt();

    return qMakePair(majorV,minorV);
}

UbuntuClickDialog::UbuntuClickDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Run Click"));

    QFormLayout* layout = new QFormLayout(this);
    this->setLayout(layout);

    m_output = new QPlainTextEdit();

    //setup the output window in the way the cmake plugin does
    //typewriter is just better for console output
    m_output->setMinimumHeight(15);
    QFont f(TextEditor::FontSettings::defaultFixedFontFamily());
    f.setStyleHint(QFont::TypeWriter);
    m_output->setFont(f);

    QSizePolicy pl = m_output->sizePolicy();
    pl.setVerticalStretch(1);
    m_output->setSizePolicy(pl);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_exitStatus = new QLabel();
    m_exitStatus->setVisible(false);

    layout->addRow(new QLabel(tr("Run Click")));
    layout->addRow(m_exitStatus);
    layout->addRow(m_output);
    layout->addRow(m_buttonBox);

    m_process = new Utils::QtcProcess(this);
    connect(m_process,SIGNAL(readyReadStandardOutput()),this,SLOT(on_clickReadyReadStandardOutput()));
    connect(m_process,SIGNAL(readyReadStandardError()),this,SLOT(on_clickReadyReadStandardError()));
    connect(m_process,SIGNAL(finished(int)),this,SLOT(on_clickFinished(int)));

    setMinimumSize(640,480);
}

UbuntuClickDialog::~UbuntuClickDialog()
{

}

void UbuntuClickDialog::setParameters(ProjectExplorer::ProcessParameters *params)
{
    params->resolveAll();
    m_process->setCommand(params->command(),params->arguments());
    m_process->setEnvironment(params->environment());
    m_process->setWorkingDirectory(params->workingDirectory());
}

void UbuntuClickDialog::runClick( )
{
    //change the button to cancel
    m_buttonBox->clear();
    m_buttonBox->addButton(QDialogButtonBox::Cancel);

    m_process->start();
}

void UbuntuClickDialog::runClickModal(ProjectExplorer::ProcessParameters *params)
{
    UbuntuClickDialog dlg;
    dlg.setParameters(params);
    QMetaObject::invokeMethod(&dlg,"runClick",Qt::QueuedConnection);
    dlg.exec();
}

void UbuntuClickDialog::createClickChrootModal()
{
    const char* supportedArchs[]  = {"armhf","i386","amd64","\0"};
    const char* supportedSeries[] = {"saucy","trusty","\0"};

    QDialog dlg;
    QFormLayout* layout = new QFormLayout(&dlg);
    dlg.setLayout(layout);

    //add supported architectures
    QComboBox* cbArch = new QComboBox(&dlg);
    for(int i = 0; supportedArchs[i][0] != '\0' ;i++) {
        cbArch->addItem(QLatin1String(supportedArchs[i]));
    }

    //add supported series
    QComboBox* cbSeries = new QComboBox(&dlg);
    for(int i = 0; supportedSeries[i][0] != '\0' ;i++) {
        cbSeries->addItem(QLatin1String(supportedSeries[i]));
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));

    layout->addRow(tr("Architecture"),cbArch);
    layout->addRow(tr("Series"),cbSeries);
    layout->addRow(buttonBox);

    if(dlg.exec() == QDialog::Accepted) {
        ProjectExplorer::ProcessParameters params;
        qDebug()<<"Creating click chroot arch: "<<cbArch->currentText()<<" series: "<<cbSeries->currentText();
        UbuntuClickTool::parametersForCreateChroot(cbArch->currentText(),cbSeries->currentText(),&params);
        runClickModal(&params);
    }
}

void UbuntuClickDialog::maintainClickModal(const UbuntuClickTool::Target &target, const UbuntuClickTool::MaintainMode &mode)
{
    if(mode == UbuntuClickTool::Delete) {
        QString title = tr("Delete click chroot");
        QString text  = tr("Are you sure you want to delete this chroot?");
        if( QMessageBox::question(Core::ICore::mainWindow(),title,text) != QMessageBox::Yes )
            return;
    }

    ProjectExplorer::ProcessParameters params;
    UbuntuClickTool::parametersForMaintainChroot(mode,target,&params);
    runClickModal(&params);
}

void UbuntuClickDialog::done(int code)
{
    if(code == QDialog::Rejected) {
        if(m_process->state() != QProcess::NotRunning) {
            //ask the user if he really wants to do that
            QString title = tr("Stop click tool");
            QString text  = tr("Are you sure you want to stop click? This could break your chroot!");
            if( QMessageBox::question(Core::ICore::mainWindow(),title,text)!= QMessageBox::Yes )
                return;

            m_process->terminate();
            m_process->waitForFinished(100);
            m_process->kill();

            m_exitStatus->setText(tr("Waiting for click to stop"));
            m_exitStatus->setVisible(true);
            return;
        }
    }
    QDialog::done(code);
}

void UbuntuClickDialog::on_clickFinished(int exitCode)
{
    //set the button to close again
    m_buttonBox->clear();
    m_buttonBox->addButton(QDialogButtonBox::Close);

    if (exitCode != 0) {
        on_clickReadyReadStandardError(tr("---Click exited with errors, please check the output---"));
    } else {
        on_clickReadyReadStandardOutput(tr("---Click exited with no errors---"));
    }
}

void UbuntuClickDialog::on_clickReadyReadStandardOutput(const QString txt)
{
    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat tf;

    QFont font = m_output->font();
    tf.setFont(font);
    tf.setForeground(m_output->palette().color(QPalette::Text));

    if(txt.isEmpty())
        cursor.insertText(QString::fromLocal8Bit(m_process->readAllStandardOutput()), tf);
    else
        cursor.insertText(txt, tf);
}

void UbuntuClickDialog::on_clickReadyReadStandardError(const QString txt)
{
    QTextCursor cursor(m_output->document());
    QTextCharFormat tf;

    QFont font = m_output->font();
    QFont boldFont = font;
    boldFont.setBold(true);
    tf.setFont(boldFont);
    tf.setForeground(QColor(Qt::red));

    if(txt.isEmpty())
        cursor.insertText(QString::fromLocal8Bit(m_process->readAllStandardError()), tf);
    else
        cursor.insertText(txt, tf);
}

/**
 * @class UbuntuClickManager
 * Build support for click chroot targets, this is a
 * temporary solution until we find a way to make cmakeplugin
 * work like we need it
 */
UbuntuClickManager::UbuntuClickManager(QObject *parent)
    : QObject(parent)
    , m_currentProject(0)
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
    qDebug()<<"Registering build menu";
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(projectExplorer, SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)),
            this, SLOT(updateSelectedProject(ProjectExplorer::Project*)));

    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    //only visible for a cmake project
    const Core::Context projectContext(CMakeProjectManager::Constants::PROJECTCONTEXT);

    m_buildInChrootAction = new QAction(QIcon(), tr("Build in chroot"), this);
    Core::Command *command = Core::ActionManager::registerAction(m_buildInChrootAction,
                                                                 Constants::UBUNTU_CLICK_BUILD_CONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);

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

        Core::FutureProgress* futureProgress = Core::ICore::progressManager()->addTask(m_futureInterface->future(),tr("Building"),QLatin1String("UbuntuClickManager.Build"));
        connect(futureProgress,SIGNAL(clicked()),this,SLOT(stop()));
    }

    m_currentBuild = m_pendingBuilds.dequeue();

    ProjectExplorer::BuildConfiguration* bc = m_currentBuild->buildTarget->activeBuildConfiguration();
    if(!bc) {
        delete m_currentBuild;
        m_currentBuild = 0;

        printToOutputPane(tr("--- Building %0 failed, No active build configuration ---").arg(m_currentBuild->buildTarget->project()->displayName()));

        QMetaObject::invokeMethod(this,"processBuildQueue",Qt::QueuedConnection);
        return;
    }

    //only one build at a time
    m_buildInChrootAction->setEnabled(false);

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
    qDebug()<<"STOP STOP STOP";
    if (m_currentBuild) {
        m_futureInterface->reportCanceled();
        m_futureInterface->reportFinished();

        if (m_process->state() != QProcess::NotRunning) {
            printToOutputPane(tr("--- Build canceled ---"));
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

    m_buildInChrootAction->setEnabled(true);
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
        m_futureInterface->setProgressValueAndText(1,tr("Cleaning old build"));
        printToOutputPane(tr("Starting to build %0").arg(m_currentBuild->buildTarget->project()->displayName()));
        printToOutputPane(tr("Using build directory %0").arg(m_currentBuild->buildDir));
        printToOutputPane(tr("Cleaning old build"));
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

        m_futureInterface->setProgressValueAndText(2,tr("Running CMake"));
        printToOutputPane(tr("Running CMake"));
        startProcess(params);
        break;
    }
    case Cmake:{
        m_currentBuild->currentState = FixMoc;

        QPair<int,int> chrootVersion = UbuntuClickTool::targetVersion(m_currentBuild->targetChroot);
        if(chrootVersion.first == -1) {
            printToOutputPane(tr("Could not find any version information in click target: click-%0-%1")
                              .arg(m_currentBuild->targetChroot.framework)
                              .arg(m_currentBuild->targetChroot.architecture));
            stop();
            return;
        }

        if(chrootVersion.first >= 14) { //the fix script needs to run only on targets older than trusty
            nextStep();
            break;
        }

        ProjectExplorer::ProcessParameters params;
        QStringList arguments;
        arguments << QLatin1String("-c")
                  << QLatin1String("find . -name AutomocInfo.cmake | xargs sed -i 's;AM_QT_MOC_EXECUTABLE .*;AM_QT_MOC_EXECUTABLE \"/usr/lib/'$(dpkg-architecture -qDEB_BUILD_MULTIARCH)'/qt5/bin/moc\");'");

        params.setWorkingDirectory(m_currentBuild->buildDir);
        params.setCommand(QLatin1String("/bin/bash"));
        params.setArguments(Utils::QtcProcess::joinArgs(arguments));
        params.setEnvironment(Utils::Environment::systemEnvironment());

        m_futureInterface->setProgressValueAndText(3,tr("Fixing build script"));
        printToOutputPane(tr("Fixing build script"));
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
        m_futureInterface->setProgressValueAndText(4,tr("Running Make"));
        printToOutputPane(tr("Running Make"));
        startProcess(params);
        break;
    }
    case Make:{
        m_currentBuild->currentState = Finished;

        delete m_currentBuild;
        m_currentBuild = 0;

        m_futureInterface->reportFinished();
        printToOutputPane(tr("--- Build was finished successfully ---"));

        //give the UI time to show we are finished
        QMetaObject::invokeMethod(this,"processBuildQueue",Qt::QueuedConnection);

        m_buildInChrootAction->setEnabled(true);
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
    printToOutputPane(tr("Running command: %0 %1").arg(params.command()).arg(params.arguments()));
    m_process->setCommand(params.command(),params.arguments());
    m_process->setWorkingDirectory(params.workingDirectory());
    m_process->setEnvironment(params.environment());
    m_process->start();
}

/**
 * @brief UbuntuClickManager::on_buildInChrootAction
 * callback slot that is called by the action
 */
void UbuntuClickManager::on_buildInChrootAction()
{
    if(!m_currentProject)
        return;

    ProjectExplorer::Target* buildTarget = m_currentProject->activeTarget();
    if(!buildTarget)
        return;

    QList<UbuntuClickTool::Target> targets = UbuntuClickTool::listAvailableTargets();
    if (!targets.size()) {
        QMessageBox::warning(Core::ICore::mainWindow()
                             ,tr("No click build targets available")
                             ,tr("There are no click build targets available.\nPlease create a target in the Ubuntu option page."));
        return;
    }

    QStringList items;
    foreach(const UbuntuClickTool::Target& t, targets)
        items << QString::fromLatin1("%0-%1").arg(t.framework).arg(t.architecture);

    bool ok = false;
    QString item = QInputDialog::getItem(Core::ICore::mainWindow()
                                         ,tr("Select build target")
                                         ,tr("Build target")
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
            printToOutputPane(tr("--- Build failed ---"));
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

void UbuntuClickManager::updateSelectedProject(ProjectExplorer::Project *project)
{
    m_currentProject = project;
}

} // namespace Internal
} // namespace Ubuntu

