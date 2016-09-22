/*
 * Copyright 2016 Canonical Ltd.
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

#include "snapcraftpackagestep.h"

#include <ubuntu/ubuntuconstants.h>
#include <ubuntu/ubuntupackageoutputparser.h>

#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/toolchain.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QTimer>
#include <QRegularExpression>

namespace Ubuntu {
namespace Internal {

const char * PACKAGE_NAME_REGEX = "^Snapped ([\\S]+\\.snap)$";

SnapcraftPackageStep::SnapcraftPackageStep(ProjectExplorer::BuildStepList *bsl)
    : ProjectExplorer::BuildStep (bsl, Constants::UBUNTU_SNAP_PACKAGESTEP_ID)
{

}

SnapcraftPackageStep::SnapcraftPackageStep(ProjectExplorer::BuildStepList *bsl, SnapcraftPackageStep *other)
    : ProjectExplorer::BuildStep (bsl, other)
{

}

SnapcraftPackageStep::~SnapcraftPackageStep()
{
    cleanup();
}

QString SnapcraftPackageStep::packagePath() const
{
    if(m_snapPackageName.isEmpty())
        return QString();
    return snapWorkingDir()
            + QDir::separator()
            + m_snapPackageName;
}

QString SnapcraftPackageStep::snapWorkingDir() const
{
    return m_buildDir+QStringLiteral("/snap-deploy");
}

bool SnapcraftPackageStep::init(QList<const ProjectExplorer::BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps)
    return true;
}

void SnapcraftPackageStep::run(QFutureInterface<bool> &fi)
{
    internalInit();

    if (m_tasks.size()) {
        foreach (const ProjectExplorer::Task& task, m_tasks) {
            addTask(task);
        }
        emit addOutput(tr("Configuration is invalid. Aborting build")
                       ,ProjectExplorer::BuildStep::MessageOutput);

        reportRunResult(fi, false);
        cleanup();
        return;
    }

    m_state = Idle;
    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0,2);
    QTimer::singleShot(0,this,SLOT(doNextStep()));
}

void SnapcraftPackageStep::cleanup()
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

    //reset params
    m_SnapReviewParam = m_MakeParam = ProjectExplorer::ProcessParameters();
    m_currParam = nullptr;
}

/*!
 * \brief UbuntuPackageStep::setupAndStartProcess
 * Setups the interal QProcess and connects the required SIGNALS
 * also makes sure the process has a clean output parser
 */
void SnapcraftPackageStep::setupAndStartProcess(ProjectExplorer::ProcessParameters &params)
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->deleteLater();
    }

    m_currParam = &params;

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

    emit addOutput(tr("Starting: \"%1 %2\"").arg(params.effectiveCommand(),params.effectiveArguments()),
                   BuildStep::MessageOutput);

    ProjectExplorer::IOutputParser *parser = target()->kit()->createOutputParser();

    //add special parser on click review step
    if(m_state == SnapReview) {
        UbuntuPackageOutputParser *packageStepParser = new UbuntuPackageOutputParser;
        //packageStepParser->setTreatAllErrorsAsWarnings(m_treatClickErrorsAsWarnings);
        connect(this,SIGNAL(currentSubStepFinished()),packageStepParser,SLOT(setEndOfData()));
        if (parser)
            parser->appendOutputParser(packageStepParser);
        else
            parser = packageStepParser;

    }

    if(m_outputParserChain) {
        delete m_outputParserChain;
        m_outputParserChain = 0;
    }

    if(parser) {
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
bool SnapcraftPackageStep::processFinished(FinishedCheckMode mode)
{
    //make sure all data has been read
    QString line = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!line.isEmpty())
        stdError(line);

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!line.isEmpty())
        stdOutput(line);

    emit currentSubStepFinished();

    bool success = true;

    if (m_outputParserChain) {
        m_outputParserChain->flush();

        if(m_outputParserChain->hasFatalErrors())
            success = false;
    }

    if(success) {
        QString command;
        if(m_currParam)
            command = QDir::toNativeSeparators(m_currParam->effectiveCommand());
        else
            command = tr("Unknown command");

        if (m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0) {
            emit addOutput(tr("The process \"%1\" exited normally.").arg(command),
                           BuildStep::MessageOutput);
        } else if (m_process->exitStatus() == QProcess::NormalExit) {
            emit addOutput(tr("The process \"%1\" exited with code %2.")
                           .arg(command, QString::number(m_process->exitCode())),
                           BuildStep::ErrorMessageOutput);
            if(mode == CheckReturnCode)
                //error
                success = false;
            else {
                emit addOutput(tr("Ignoring return code for this step"),BuildStep::ErrorMessageOutput);
            }
        } else {
            emit addOutput(tr("The process \"%1\" crashed.").arg(command), BuildStep::ErrorMessageOutput);

            //error
            success = false;
        }
    }

    //the process failed, lets clean up
    if (!success) {
        if(m_futureInterface)
            reportRunResult(*m_futureInterface, false);
        cleanup();
    }
    return success;
}

void SnapcraftPackageStep::stdOutput(const QString &line)
{
    m_lastLine = line;

    if (m_outputParserChain)
        m_outputParserChain->stdOutput(line);
    emit addOutput(line, BuildStep::NormalOutput, BuildStep::DontAppendNewline);
}

void SnapcraftPackageStep::stdError(const QString &line)
{
    if (m_outputParserChain)
        m_outputParserChain->stdError(line);
    emit addOutput(line, BuildStep::ErrorOutput, BuildStep::DontAppendNewline);
}


void SnapcraftPackageStep::onProcessStdOut()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdOutput(line);
    }
}

void SnapcraftPackageStep::onProcessStdErr()
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdError(line);
    }
}

void SnapcraftPackageStep::outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format)
{
    emit addOutput(string, format, BuildStep::DontAppendNewline);
}

void SnapcraftPackageStep::taskAdded(const ProjectExplorer::Task &task)
{
    emit addTask(task);
}

void SnapcraftPackageStep::onProcessFailedToStart()
{
    if(m_futureInterface)
        reportRunResult(*m_futureInterface, false);

    QString command = tr("Unknown command");
    QString args;

    if (m_currParam) {
        command = QDir::toNativeSeparators(m_currParam->effectiveCommand());
        args    = m_currParam->prettyArguments();
    }

    emit addOutput(tr("Could not start process \"%1\" %2")
                   .arg(command, args),
                   BuildStep::ErrorMessageOutput);

    cleanup();

}

QString SnapcraftPackageStep::makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const
{
    if (tc)
        return tc->makeCommand(env);
    return QString();
}


ProjectExplorer::BuildStepConfigWidget *SnapcraftPackageStep::createConfigWidget()
{
    return nullptr;
}

void SnapcraftPackageStep::cancel()
{

}

bool SnapcraftPackageStep::immutable() const
{
    return true;
}

bool SnapcraftPackageStep::runInGuiThread() const
{
    return true;
}

void SnapcraftPackageStep::doNextStep()
{
    switch (m_state) {
        case Idle: {
            m_state = MakeSnap;
            m_currParam = nullptr;
            setupAndStartProcess(m_MakeParam);
            break;
        }
        case MakeSnap: {
            if (!processFinished(CheckReturnCode))
                return;

            m_currParam = nullptr;

            QRegularExpression exp((QLatin1String(PACKAGE_NAME_REGEX)));
            QRegularExpressionMatch m = exp.match(m_lastLine);
            if(m.hasMatch()) {
                m_snapPackageName = m.captured(1);
                emit addOutput(tr("The click package has been created in %1").arg(snapWorkingDir()) ,
                               ProjectExplorer::BuildStep::MessageOutput);
            }

            m_futureInterface->setProgressValueAndText(1,tr("Reviewing snap package"));
            m_state = SnapReview;

            m_SnapReviewParam.setArguments(QString::fromLatin1(Constants::CLICK_REVIEWERSTOOLS_ARGS).arg(packagePath()));
            m_SnapReviewParam.resolveAll();
            setupAndStartProcess(m_SnapReviewParam);
            break;
        }
        case SnapReview: {
            //we need to ignore the return code for now,
            //until we have proper support for ignoring specific errors
            if (!processFinished(IgnoreReturnCode))
                return;

            if(m_futureInterface)
                reportRunResult(*m_futureInterface, true);
            cleanup();
            break;
        }
    }
}

void SnapcraftPackageStep::internalInit()
{
    m_tasks.clear();

    QString projectDir = target()->project()->projectDirectory().toString();
    m_buildDir.clear();
    Utils::Environment env = Utils::Environment::systemEnvironment();
    Utils::MacroExpander *mExp = 0;


    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();
    if (!bc) {
        ProjectExplorer::Task t(ProjectExplorer::Task::Error
                                ,tr("No valid BuildConfiguration set for step: %1").arg(displayName())
                                ,Utils::FileName(),-1
                                ,ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        m_tasks.append(t);

        //SnapcraftPackageStep::run will stop if tasks exist
        return;

    }

    m_buildDir  = bc->buildDirectory().toString();
    env = bc->environment();
    mExp = bc->macroExpander();

    {
        QStringList arguments {
            QStringLiteral("snap")
        };

        ProjectExplorer::ProcessParameters* params = &m_MakeParam;
        params->setMacroExpander(mExp);

        //setup process parameters
        params->setWorkingDirectory(m_buildDir);
        params->setCommand(
                    makeCommand(ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit()),
                                env));
        params->setArguments(Utils::QtcProcess::joinArgs(arguments));

        Utils::Environment tmpenv = env;
        // Force output to english for the parsers. Do this here and not in the toolchain's
        // addToEnvironment() to not screw up the users run environment.
        tmpenv.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        params->setEnvironment(tmpenv);

        params->resolveAll();
    }

    //builds the snap review arguments
    {
        ProjectExplorer::ProcessParameters* params = &m_SnapReviewParam;
        params->setMacroExpander(mExp);

        //setup process parameters
        params->setWorkingDirectory(m_buildDir);
        params->setCommand(QLatin1String(Constants::CLICK_REVIEWERSTOOLS_BINARY));

        Utils::Environment tmpEnv = env;
        // Force output to english for the parsers. Do this here and not in the toolchain's
        // addToEnvironment() to not screw up the users run environment.
        tmpEnv.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        params->setEnvironment(tmpEnv);
    }
}


} // namespace Internal
} // namespace Ubuntu
