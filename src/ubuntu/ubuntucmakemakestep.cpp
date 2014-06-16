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
#include "ubuntucmakemakestep.h"
#include "ubuntuconstants.h"
#include "ubuntuprojectguesser.h"
#include "clicktoolchain.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/customparser.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <utils/qtcassert.h>

#include <QTimer>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuCMakeMakeStepFactory
 * Factory class to create UbuntuCMakeMakeStep
 * build steps
 */
QList<Core::Id> UbuntuCMakeMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if(!canHandle(parent->target()))
        return QList<Core::Id>();

    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY) {
        return QList<Core::Id>()
                << Core::Id(Constants::UBUNTU_CLICK_PACKAGESTEP_ID);
    }

    return QList<Core::Id>() << Core::Id(Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID);
}

/*!
 * \brief UbuntuCMakeBuildConfigurationFactory::canHandle
 * checks if we can create buildconfigurations for the given target
 */
bool UbuntuCMakeMakeStepFactory::canHandle(const ProjectExplorer::Target *t) const
{
    QTC_ASSERT(t, return false);
    if (!t->project()->supportsKit(t->kit()))
        return false;

    if(ProjectExplorer::DeviceKitInformation::deviceId(t->kit()) == ProjectExplorer::Constants::DESKTOP_DEVICE_ID) {
        return UbuntuProjectGuesser::isClickAppProject(t->project());
    }

    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(t->kit());
    if(!tc || tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
        return false;

    return t->project()->id() == Core::Id(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
}

QString UbuntuCMakeMakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID)
        return tr("UbuntuSDK-Make", "Display name for UbuntuCMakeMakeStep id.");
    if (id == Constants::UBUNTU_CLICK_PACKAGESTEP_ID)
        return tr("UbuntuSDK create click package", "Display name for UbuntuPackageStep id.");
    return QString();
}

bool UbuntuCMakeMakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if (canHandle(parent->target()))
        return availableCreationIds(parent).contains(id);
    return false;
}

ProjectExplorer::BuildStep *UbuntuCMakeMakeStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    if (id == Constants::UBUNTU_CLICK_PACKAGESTEP_ID) {
        UbuntuPackageStep *step = new UbuntuPackageStep(parent);
        return step;
    }

    UbuntuCMakeMakeStep *step = new UbuntuCMakeMakeStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setUseNinja(false);
        step->setClean(true);
        step->setAdditionalArguments(QLatin1String("clean"));
    }
    return step;
}

bool UbuntuCMakeMakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    Core::Id toRestore = ProjectExplorer::idFromMap(map);

    //backwards compatibility to older projects
    if( toRestore == Constants::UBUNTU_DEPLOY_MAKESTEP_ID )
        return canHandle(parent->target());

    return canCreate(parent, toRestore);
}

ProjectExplorer::BuildStep *UbuntuCMakeMakeStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Core::Id toRestore = ProjectExplorer::idFromMap(map);

    //backwards compatibility to older projects
    if( toRestore == Constants::UBUNTU_DEPLOY_MAKESTEP_ID ) {
        UbuntuPackageStep *step = new UbuntuPackageStep(parent);
        return step;
    } else {
        ProjectExplorer::BuildStep* step = create(parent,toRestore);
        if(step->fromMap(map))
            return step;

        delete step;
    }
    return 0;
}

bool UbuntuCMakeMakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *UbuntuCMakeMakeStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;

    if(product->id() == Core::Id(Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID))
        return new UbuntuCMakeMakeStep(parent, static_cast<UbuntuCMakeMakeStep *>(product));
    else if(product->id() == Core::Id(Constants::UBUNTU_CLICK_PACKAGESTEP_ID))
        return new UbuntuPackageStep(parent, static_cast<UbuntuPackageStep *>(product));

    QTC_ASSERT(false,return 0);
}

/*!
 * \class UbuntuCMakeMakeStep
 * Represents a make or make clean call in the Ubuntu-SDK build chain
 */
UbuntuCMakeMakeStep::UbuntuCMakeMakeStep(ProjectExplorer::BuildStepList *bsl)
    : MakeStep(bsl,Core::Id(Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID))
{
    setDefaultDisplayName(tr("Ubuntu SDK Make"));
}

UbuntuCMakeMakeStep::UbuntuCMakeMakeStep(ProjectExplorer::BuildStepList *bsl, UbuntuCMakeMakeStep *bs)
    : MakeStep(bsl,bs)
{
}

UbuntuCMakeMakeStep::~UbuntuCMakeMakeStep()
{

}

QString UbuntuCMakeMakeStep::makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const
{
    if (tc)
        return tc->makeCommand(env);
    return QString::fromLatin1(Constants::UBUNTU_CLICK_MAKE_WRAPPER).arg(Constants::UBUNTU_SCRIPTPATH);
}

UbuntuPackageStep::UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl,Constants::UBUNTU_CLICK_PACKAGESTEP_ID),
    m_state(Idle),
    m_futureInterface(0),
    m_process(0),
    m_outputParserChain(0)
{
    setDefaultDisplayName(tr("UbuntuSDK Click build"));
}

UbuntuPackageStep::UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl, UbuntuPackageStep *other) :
    ProjectExplorer::BuildStep(bsl,other),
    m_state(Idle),
    m_futureInterface(0),
    m_process(0),
    m_outputParserChain(0)
{

}

UbuntuPackageStep::~UbuntuPackageStep()
{
    cleanup();
}

bool UbuntuPackageStep::init()
{
    m_tasks.clear();

    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();
    if (!bc){
        ProjectExplorer::Task t(ProjectExplorer::Task::Error
                                ,tr("No valid BuildConfiguration set for step: %1").arg(displayName())
                                ,Utils::FileName(),-1
                                ,ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        m_tasks.append(t);

        //UbuntuClickPackageStep::run will stop if tasks exist
        return true;

    }

    //build the make process arguments
    {
        QStringList arguments;
        arguments << QStringLiteral("DESTDIR=%1").arg(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                  << QStringLiteral("install");

        ProjectExplorer::ProcessParameters* params = &m_MakeParam;
        params->setMacroExpander(bc->macroExpander());

        //setup process parameters
        params->setWorkingDirectory(bc->buildDirectory().toString());
        params->setCommand(
                    makeCommand(ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit()),
                                bc->environment()));
        params->setArguments(Utils::QtcProcess::joinArgs(arguments));

        Utils::Environment env = bc->environment();
        // Force output to english for the parsers. Do this here and not in the toolchain's
        // addToEnvironment() to not screw up the users run environment.
        env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        params->setEnvironment(env);

        params->resolveAll();
    }


    //builds the click process arguments
    {
        QStringList arguments;
        arguments << QLatin1String("build")
                  << bc->buildDirectory().toString()
                     + QDir::separator()
                     + QString::fromLatin1(Constants::UBUNTU_DEPLOY_DESTDIR);

        ProjectExplorer::ProcessParameters* params = &m_ClickParam;
        params->setMacroExpander(bc->macroExpander());

        //setup process parameters
        params->setWorkingDirectory(bc->buildDirectory().toString());
        params->setCommand(QLatin1String("click"));
        params->setArguments(Utils::QtcProcess::joinArgs(arguments));

        Utils::Environment env = bc->environment();
        // Force output to english for the parsers. Do this here and not in the toolchain's
        // addToEnvironment() to not screw up the users run environment.
        env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        params->setEnvironment(env);

        params->resolveAll();
    }
    return true;
}

void UbuntuPackageStep::run(QFutureInterface<bool> &fi)
{
    if (m_tasks.size()) {
        foreach (const ProjectExplorer::Task& task, m_tasks) {
            addTask(task);
        }
        emit addOutput(tr("Configuration is invalid. Aborting build")
                       ,ProjectExplorer::BuildStep::MessageOutput);
        fi.reportResult(false);
        emit finished();
        return;
    }

    m_state = Idle;
    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0,3);
    QTimer::singleShot(0,this,SLOT(doNextStep()));
}

ProjectExplorer::BuildStepConfigWidget *UbuntuPackageStep::createConfigWidget()
{
    return new ProjectExplorer::SimpleBuildStepConfigWidget(this);
}

bool UbuntuPackageStep::immutable() const
{
    //do not allow the user to reorder or to delete the packaging step
    return true;
}

bool UbuntuPackageStep::runInGuiThread() const
{
    return true;
}

bool UbuntuPackageStep::fromMap(const QVariantMap &map)
{
    return BuildStep::fromMap(map);
}

QVariantMap UbuntuPackageStep::toMap() const
{
    return BuildStep::toMap();
}

QString UbuntuPackageStep::packagePath() const
{
    if(m_clickPackageName.isEmpty())
        return QString();
    return target()->activeBuildConfiguration()->buildDirectory().toString()
            + QDir::separator()
            + m_clickPackageName;
}

/*!
 * \brief UbuntuPackageStep::setupAndStartProcess
 * Setups the interal QProcess and connects the required SIGNALS
 * also makes sure the process has a clean output parser
 */
void UbuntuPackageStep::setupAndStartProcess(const ProjectExplorer::ProcessParameters &params)
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->deleteLater();
    }

    QDir wd(params.effectiveWorkingDirectory());
    if (!wd.exists())
        wd.mkpath(wd.absolutePath());

    QString effectiveCommand = params.effectiveCommand();
    if (!QFileInfo(effectiveCommand).exists()) {
        onProcessFailedToStart();
        return;
    }

    m_process = new Utils::QtcProcess();
    connect(m_process,SIGNAL(finished(int)),this,SLOT(doNextStep()));
    connect(m_process,SIGNAL(readyReadStandardOutput()),this,SLOT(onProcessStdOut()));
    connect(m_process,SIGNAL(readyReadStandardError()),this,SLOT(onProcessStdErr()));

    m_process->setCommand(params.effectiveCommand(),params.effectiveArguments());
    m_process->setEnvironment(params.environment());
    m_process->setWorkingDirectory(wd.absolutePath());

    qDebug()<<"Starting process "<<params.effectiveCommand()<<params.effectiveArguments();

    ProjectExplorer::IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser) {
        if(m_outputParserChain)
            delete m_outputParserChain;

        m_outputParserChain = parser;
        m_outputParserChain->setWorkingDirectory(params.effectiveWorkingDirectory());

        connect(m_outputParserChain,SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat)),
                this,SLOT(outputAdded(QString,ProjectExplorer::BuildStep::OutputFormat)));
        connect(m_outputParserChain,SIGNAL(addTask(ProjectExplorer::Task)),
                this,SLOT(taskAdded(ProjectExplorer::Task)));
    }

    m_process->start();
    if(!m_process->waitForStarted()) {
        onProcessFailedToStart();
        return;
    }
}

/*!
 * \brief UbuntuPackageStep::checkLastProcessSuccess
 * Checks if the last process has run without any errors
 */
bool UbuntuPackageStep::processFinished()
{
    //make sure all data has been read
    QString line = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!line.isEmpty())
        stdError(line);

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!line.isEmpty())
        stdOutput(line);

    if (m_outputParserChain) {
        m_outputParserChain->flush();

        if(m_outputParserChain->hasFatalErrors())
            return false;
    }

    QString command;
    if(m_state == MakeInstall)
        command = QDir::toNativeSeparators(m_MakeParam.effectiveCommand());
    else
        command = QDir::toNativeSeparators(m_ClickParam.effectiveCommand());

    bool success = true;
    if (m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0) {
        emit addOutput(tr("The process \"%1\" exited normally.").arg(command),
                       BuildStep::MessageOutput);
    } else if (m_process->exitStatus() == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(command, QString::number(m_process->exitCode())),
                       BuildStep::ErrorMessageOutput);
        //error
        success = false;
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(command), BuildStep::ErrorMessageOutput);

        //error
        success = false;
    }

    //the process failed, lets clean up
    if (!success) {
        m_futureInterface->reportResult(false);
        cleanup();
        emit finished();
    }

    return success;
}

void UbuntuPackageStep::cleanup()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->deleteLater();
        m_process = 0;
    }

    //not owned by us
    m_futureInterface = 0;

    if (m_outputParserChain) {
        delete m_outputParserChain;
        m_outputParserChain = 0;
    }
}

void UbuntuPackageStep::stdOutput(const QString &line)
{
    m_lastLine = line;

    if (m_outputParserChain)
        m_outputParserChain->stdOutput(line);
    emit addOutput(line, BuildStep::NormalOutput, BuildStep::DontAppendNewline);
}

void UbuntuPackageStep::stdError(const QString &line)
{
    if (m_outputParserChain)
        m_outputParserChain->stdError(line);
    emit addOutput(line, BuildStep::ErrorOutput, BuildStep::DontAppendNewline);
}

QString UbuntuPackageStep::makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const
{
    if (tc)
        return tc->makeCommand(env);
    return QString::fromLatin1(Constants::UBUNTU_CLICK_MAKE_WRAPPER).arg(Constants::UBUNTU_SCRIPTPATH);
}

/*!
 * \brief UbuntuPackageStep::injectDebugHelperStep
 * Checks if its required to inject the debug helpers and does that
 * accordingly
 */
void UbuntuPackageStep::injectDebugHelperStep()
{
    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();

    bool ubuntuDevice = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(bc->target()->kit()) == Constants::UBUNTU_DEVICE_TYPE_ID;
    if( bc->buildType() == ProjectExplorer::BuildConfiguration::Debug && ubuntuDevice ) {
        QRegularExpression fileRegEx(QStringLiteral("^.*\\.desktop$"));
        QList<Utils::FileName> desktopFiles = UbuntuProjectGuesser::findFilesRecursive(bc->buildDirectory().
                                                                                       appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR)),
                                                                                       fileRegEx);


        QRegularExpression regex(QStringLiteral("^(\\s*[Ee][Xx][Ee][cC]=.*)$"),QRegularExpression::MultilineOption);
        foreach(const Utils::FileName &deskFile, desktopFiles) {
            QFile deskFileFd(deskFile.toFileInfo().absoluteFilePath());
            if(!deskFileFd.open(QIODevice::ReadOnly))
                continue;

            QString contents;
            {
                QTextStream deskInStream(&deskFileFd);
                contents = deskInStream.readAll();
            }
            deskFileFd.close();

            QRegularExpressionMatch m = regex.match(contents);
            if(!m.hasMatch())
                continue;

            QString exec = m.captured(1);
            int idxOfEq = exec.indexOf(QStringLiteral("="));
            exec.remove(0,idxOfEq+1);

            contents.replace(m.capturedStart(1),
                             m.capturedLength(1),
                             QStringLiteral("Exec=./qtc_device_debughelper.py \"%1\"").arg(exec));


            if(!deskFileFd.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                continue;
            }

            QTextStream outStream(&deskFileFd);
            outStream << contents;
            deskFileFd.close();
        }

        const QString debScript = QStringLiteral("qtc_device_debughelper.py");
        const QString debSourcePath = QStringLiteral("%1/%2").arg(Constants::UBUNTU_SCRIPTPATH).arg(debScript);
        const QString debTargetPath = QStringLiteral("%1/%2/%3")
                .arg(bc->buildDirectory().toString())
                .arg(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                .arg(debScript);

        if(QFile::exists(debTargetPath))
            QFile::remove(debTargetPath);
        if(QFile::exists(debSourcePath))
            QFile::copy(debSourcePath,debTargetPath);
    }

    QTimer::singleShot(0,this,SLOT(doNextStep()));
}

void UbuntuPackageStep::doNextStep()
{
    switch (m_state) {
        case Idle: {
            m_futureInterface->setProgressValueAndText(0,tr("Make install"));

            m_state = MakeInstall;
            setupAndStartProcess(m_MakeParam);
            break;
        }
        case MakeInstall: {
            if (!processFinished())
                return;

            m_futureInterface->setProgressValueAndText(1,tr("Preparing click package tree"));
            m_state = PreparePackage;

            //return to MainLoop to update the progressBar
            QTimer::singleShot(0,this,SLOT(injectDebugHelperStep()));
                    break;
        }
        case PreparePackage: {
            m_futureInterface->setProgressValueAndText(2,tr("Building click package"));
            m_state = ClickBuild;
            m_lastLine.clear();
            m_clickPackageName.clear();

            setupAndStartProcess(m_ClickParam);
            break;
        }
        case ClickBuild: {
            if (!processFinished())
                return;

            QRegularExpression exp(QLatin1String(Constants::UBUNTU_CLICK_SUCCESS_PACKAGE_REGEX));
            QRegularExpressionMatch m = exp.match(m_lastLine);
            if(m.hasMatch()) {
                m_clickPackageName = m.captured(1);
                emit addOutput(tr("The click package has been created in %1").arg(target()->activeBuildConfiguration()->buildDirectory().toString()) ,
                               ProjectExplorer::BuildStep::MessageOutput);
            }

            m_futureInterface->reportResult(true);
            cleanup();
            emit finished();

            break;
        }

        default:
            break;
    }
}

void UbuntuPackageStep::onProcessStdOut()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdOutput(line);
    }
}

void UbuntuPackageStep::onProcessStdErr()
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdError(line);
    }
}

void UbuntuPackageStep::onProcessFailedToStart()
{
    m_futureInterface->reportResult(false);

    ProjectExplorer::ProcessParameters *params;
    if (m_state == MakeInstall)
        params = &m_MakeParam;
    else
        params = &m_ClickParam;

    emit addOutput(tr("Could not start process \"%1\" %2")
                   .arg(QDir::toNativeSeparators(params->effectiveCommand()),
                        params->prettyArguments()),
                   BuildStep::ErrorMessageOutput);

    emit finished();
    cleanup();

}

void UbuntuPackageStep::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)
{
    emit addOutput(string, format, BuildStep::DontAppendNewline);
}

void UbuntuPackageStep::taskAdded(const ProjectExplorer::Task &task)
{
    emit addTask(task);
}

} // namespace Internal
} // namespace Ubuntu
