#include "ubuntupackagestep.h"
#include "ui_ubuntupackagestepconfigwidget.h"
#include "ubuntuconstants.h"
#include "clicktoolchain.h"
#include "ubuntuprojecthelper.h"
#include "ubuntuclickmanifest.h"
#include "ubuntupackageoutputparser.h"
#include "settings.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <utils/qtcprocess.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <QDir>
#include <QTimer>
#include <QRegularExpression>
#include <QVariant>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

const char PACKAGE_MODE_KEY[] = "Ubuntu.UbuntuPackageStep.PackageMode";
const char ERROR_MODE_KEY[] = "Ubuntu.UbuntuPackageStep.TreatErrorsAsWarnings";

/*!
 * \class UbuntuPackageStep
 *
 * Deploy step that is responsible for running "make install",
 * injecting debug helper into the package and creating the click
 * package itself
 */

UbuntuPackageStep::UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl,Constants::UBUNTU_CLICK_PACKAGESTEP_ID),
    m_state(Idle),
    m_futureInterface(0),
    m_process(0),
    m_outputParserChain(0),
    m_debugMode(EnableDebugScript),
    m_packageMode(Default),
    m_cleanDeployDirectory(true)
{
    setDefaultDisplayName(tr("UbuntuSDK Click build"));

    m_treatClickErrorsAsWarnings = Settings::projectDefaults().reviewErrorsAsWarnings;

    if (!Settings::projectDefaults().enableDebugHelper)
        m_debugMode = DisableDebugScript;
}

UbuntuPackageStep::UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl, UbuntuPackageStep *other) :
    ProjectExplorer::BuildStep(bsl,other),
    m_state(Idle),
    m_futureInterface(0),
    m_process(0),
    m_outputParserChain(0),
    m_debugMode(other->m_debugMode),
    m_packageMode(other->m_packageMode),
    m_cleanDeployDirectory(other->m_cleanDeployDirectory)
{

}

UbuntuPackageStep::~UbuntuPackageStep()
{
    cleanup();
}

bool UbuntuPackageStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    //initialization happens in internalInit,
    //because it requires informations that are only available at this
    //time
    //@TODO refactor into single buildsteps per projecttype
    return true;
}

void UbuntuPackageStep::internalInit()
{
    m_tasks.clear();

    QString projectDir = target()->project()->projectDirectory().toString();
    m_buildDir.clear();
    m_deployDir.clear();
    Utils::Environment env = Utils::Environment::systemEnvironment();
    Utils::MacroExpander *mExp = 0;

    m_MakeParam = m_ClickParam = m_ReviewParam = ProjectExplorer::ProcessParameters();
    m_clickPackageName.clear();

    bool isCMake  = target()->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID;
    bool isQMake  = target()->project()->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID;

    {
        ProjectExplorer::BuildConfiguration *bc = referenceBuildConfig();
        if(bc) {
            m_buildDir  = bc->buildDirectory().toString();
            m_deployDir = bc->buildDirectory().toString()
                    + QDir::separator()
                    + QString::fromLatin1(Constants::UBUNTU_DEPLOY_DESTDIR);
            env = bc->environment();
            mExp = bc->macroExpander();
        } else {
            //cmake and qmake projects NEED a buildconfiguration
            if (isCMake || isQMake) {
                ProjectExplorer::Task t(ProjectExplorer::Task::Error
                                        ,tr("No valid BuildConfiguration set for step: %1").arg(displayName())
                                        ,Utils::FileName(),-1
                                        ,ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
                m_tasks.append(t);

                //UbuntuClickPackageStep::run will stop if tasks exist
                return;
            } else {
                //backward compatibility, old HTML5 projects did not have a Buildconfiguration
                //this would crash otherwise

                //ubuntu + qml project types
                QDir pDir(projectDir);
                m_buildDir = QDir::cleanPath(target()->project()->projectDirectory()
                                             .appendPath(QStringLiteral(".."))
                                             .appendPath(pDir.dirName()+QStringLiteral("_build"))
                                             .toString());
                m_deployDir = m_buildDir+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR);

                //clean up the old "build"
                QDir bd(m_buildDir);
                if(!bd.exists(m_buildDir))
                    bd.mkdir(m_buildDir);
            }
        }

        if (!m_overrideDeployDir.isEmpty())
            m_deployDir = m_overrideDeployDir;

    }

    //build the make process arguments
    {
        if (isCMake || isQMake) {
            QStringList arguments;

            if(isCMake) {
                arguments << QStringLiteral("DESTDIR=%1").arg(m_deployDir)
                          << QStringLiteral("install");
            } else {
                arguments << QStringLiteral("INSTALL_ROOT=%1").arg(m_deployDir)
                          << QStringLiteral("install");
            }

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
        } else {
            //QML and HTML projects are just rsynced for now
            QStringList arguments;
            arguments << QStringLiteral("-avh")
                      //<< QStringLiteral("--delete")
                      << QStringLiteral("--exclude")<<QStringLiteral(".bzr")
                      << QStringLiteral("--exclude")<<QStringLiteral(".git")
                      << QStringLiteral("--exclude")<<QStringLiteral(".hg")
                      << QStringLiteral("--exclude")<<QStringLiteral(".svn")
                      << QStringLiteral("--exclude")<<QStringLiteral("*.qmlproject")
                      << QStringLiteral("--exclude")<<QStringLiteral("*.user")
                      << QStringLiteral("--exclude")<<QStringLiteral("tests")
                      << QStringLiteral("--exclude")<<QStringLiteral("Makefile")
                      << QStringLiteral("--exclude")<<QStringLiteral(".excludes")
                      << QStringLiteral("--exclude")<<QStringLiteral("*.ubuntuhtmlproject")
                      << QString(QStringLiteral("--exclude-from=%1")).arg(projectDir+QDir::separator()+QStringLiteral(".excludes"));

            arguments << projectDir+QDir::separator();

            QString translationsDir = m_buildDir
                         + QDir::separator()
                         + QString::fromLatin1(Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_DIR)
                         + QDir::separator();

            if (QDir(translationsDir).exists())
                arguments << translationsDir;

            arguments << m_deployDir;

            ProjectExplorer::ProcessParameters* params = &m_MakeParam;
            params->setMacroExpander(mExp);

            //setup process parameters
            params->setWorkingDirectory(m_buildDir);
            params->setCommand(QStringLiteral("rsync"));
            params->setArguments(Utils::QtcProcess::joinArgs(arguments));

            Utils::Environment tmpenv = env;
            // Force output to english for the parsers. Do this here and not in the toolchain's
            // addToEnvironment() to not screw up the users run environment.
            tmpenv.set(QLatin1String("LC_ALL"), QLatin1String("C"));
            params->setEnvironment(tmpenv);

            params->resolveAll();
        }
    }


    //builds the click process arguments
    {
        QStringList arguments;
        arguments << QStringLiteral("build")
                  << QStringLiteral("--no-validate")
                  << m_deployDir;

        ProjectExplorer::ProcessParameters* params = &m_ClickParam;
        params->setMacroExpander(mExp);

        //setup process parameters
        params->setWorkingDirectory(clickWorkingDir());
        params->setCommand(QLatin1String("click"));
        params->setArguments(Utils::QtcProcess::joinArgs(arguments));

        Utils::Environment tmpEnv = env;
        // Force output to english for the parsers. Do this here and not in the toolchain's
        // addToEnvironment() to not screw up the users run environment.
        tmpEnv.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        params->setEnvironment(tmpEnv);

        params->resolveAll();
    }

    //builds the click review arguments
    {
        ProjectExplorer::ProcessParameters* params = &m_ReviewParam;
        params->setMacroExpander(mExp);

        //setup process parameters
        params->setWorkingDirectory(clickWorkingDir());
        params->setCommand(QLatin1String(Constants::CLICK_REVIEWERSTOOLS_BINARY));

        Utils::Environment tmpEnv = env;
        // Force output to english for the parsers. Do this here and not in the toolchain's
        // addToEnvironment() to not screw up the users run environment.
        tmpEnv.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        params->setEnvironment(tmpEnv);
    }

    return;
}

void UbuntuPackageStep::run(QFutureInterface<bool> &fi)
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
    m_futureInterface->setProgressRange(0,4);
    QTimer::singleShot(0,this,SLOT(doNextStep()));
}

ProjectExplorer::BuildStepConfigWidget *UbuntuPackageStep::createConfigWidget()
{
    return new UbuntuPackageStepConfigWidget(this);
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
    if(!BuildStep::fromMap(map))
        return false;


    setDebugMode(AutoEnableDebugScript);
    if (map.contains(QLatin1String(PACKAGE_MODE_KEY))) {
        int mode = map[QLatin1String(PACKAGE_MODE_KEY)].toInt();

        //AutoEnableDebugScript is deprecated, we always want the helper packaged
        if(mode == AutoEnableDebugScript)
            mode = EnableDebugScript;

        if(mode >= AutoEnableDebugScript && mode <= DisableDebugScript)
            setDebugMode(static_cast<DebugMode>(mode));
    }
    if (map.contains(QLatin1String(ERROR_MODE_KEY))) {
        setTreatClickErrorsAsWarnings(map[QLatin1String(ERROR_MODE_KEY)].toBool());
    }
    return true;
}

QVariantMap UbuntuPackageStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    if(map.isEmpty())
        return map;

    map.insert(QLatin1String(PACKAGE_MODE_KEY),static_cast<int>(m_debugMode));
    map.insert(QLatin1String(ERROR_MODE_KEY),m_treatClickErrorsAsWarnings);
    return map;
}

QString UbuntuPackageStep::packagePath() const
{
    if(m_clickPackageName.isEmpty())
        return QString();
    return clickWorkingDir()
            + QDir::separator()
            + m_clickPackageName;
}

UbuntuPackageStep::DebugMode UbuntuPackageStep::debugMode() const
{
    return m_debugMode;
}

void UbuntuPackageStep::setDebugMode(UbuntuPackageStep::DebugMode arg)
{
    if (m_debugMode != arg) {
        m_debugMode = arg;
        emit packageModeChanged(arg);
    }
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

    emit addOutput(tr("Starting: \"%1 %2\"").arg(params.effectiveCommand(),params.effectiveArguments()),
                   BuildStep::MessageOutput);

    ProjectExplorer::IOutputParser *parser = target()->kit()->createOutputParser();

    //add special parser on click review step
    if(m_state == ClickReview) {
        UbuntuPackageOutputParser *packageStepParser = new UbuntuPackageOutputParser;
        packageStepParser->setTreatAllErrorsAsWarnings(m_treatClickErrorsAsWarnings);
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
bool UbuntuPackageStep::processFinished(FinishedCheckMode mode)
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
        if(m_state == MakeInstall)
            command = QDir::toNativeSeparators(m_MakeParam.effectiveCommand());
        else
            command = QDir::toNativeSeparators(m_ClickParam.effectiveCommand());

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

    //reset params
    m_MakeParam = m_ClickParam = m_ReviewParam = ProjectExplorer::ProcessParameters();
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
    return QString();
}

/*!
 * \brief UbuntuPackageStep::injectDebugHelperStep
 * Checks if its required to inject the debug helpers and does that
 * accordingly
 */
void UbuntuPackageStep::injectDebugHelperStep()
{
    ProjectExplorer::BuildConfiguration *bc = referenceBuildConfig();

    if(!bc) {
        QTimer::singleShot(0,this,SLOT(doNextStep()));
        return;
    }

    bool ubuntuDevice = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(bc->target()->kit()).toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID));
    bool injectDebugScript = (m_debugMode == EnableDebugScript || m_debugMode == AutoEnableDebugScript);

    //debughelper script name, source and destination path
    const QString debScript = QStringLiteral("qtc_device_debughelper.py");
    const QString debSourcePath = QStringLiteral("%1/%2").arg(Constants::UBUNTU_SCRIPTPATH).arg(debScript);

    if(ubuntuDevice) {
        QRegularExpression deskExecRegex(QStringLiteral("^(\\s*[Ee][Xx][Ee][cC]=.*)$"),QRegularExpression::MultilineOption);

        UbuntuClickManifest manifest;
        QString err;
        QString manifestFileName = Utils::FileName::fromString(m_deployDir)
                .appendPath(QStringLiteral("manifest.json")).toString();

        if(!QFile::exists(manifestFileName)) {
            emit addOutput(tr("Could not find the manifest.json file in %1.\nPlease check if it is added to the install targets in your project file")
                           .arg(m_deployDir),
                           BuildStep::ErrorMessageOutput);

            if(m_futureInterface)
                reportRunResult(*m_futureInterface, false);
            cleanup();
            return;
        }

        if(!manifest.load(manifestFileName,nullptr,&err)) {
            emit addOutput(tr("Could not open the manifest.json file in %1.\n %2")
                           .arg(m_deployDir)
                           .arg(err),
                           BuildStep::ErrorMessageOutput);

            if(m_futureInterface)
                reportRunResult(*m_futureInterface, false);
            cleanup();
            return;
        }

        QList<UbuntuClickManifest::Hook> hooks = manifest.hooks();
        foreach ( const UbuntuClickManifest::Hook &hook, hooks) {

            UbuntuClickManifest appArmor;
            if (!appArmor.load(Utils::FileName::fromString(m_deployDir)
                               .appendPath(hook.appArmorFile)
                               .toString())) {
                qWarning()<<"Could not open the apparmor file for: "<<hook.appId;
                continue;
            }

            if (hook.desktopFile.isEmpty() && !hook.scope.isEmpty()) {
                //this is a scope hook
                const QString iniFilePath(Utils::FileName::fromString(m_deployDir)
                                          .appendPath(hook.scope)
                                          .appendPath(manifest.name()+QStringLiteral("_")+hook.appId+QStringLiteral(".ini"))
                                          .toString());

                const QString debTargetPath = Utils::FileName::fromString(m_deployDir)
                        .appendPath(hook.scope)
                        .appendPath(debScript).toString();

                //make sure there is no old script in case we don't want to package it
                if(QFile::exists(debTargetPath))
                    QFile::remove(debTargetPath);

                if(!QFile::exists(iniFilePath))
                    continue;

                if(!injectDebugScript)
                    continue;

                ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
                if(!tc || tc->typeId() != Constants::UBUNTU_CLICK_TOOLCHAIN_ID) {
                    qWarning()<<"Incompatible Toolchain for hook"<<hook.appId;
                    continue;
                }

                QString defaultSubCmd = QStringLiteral("/usr/lib/%1/unity-scopes/scoperunner %R %S ").arg(static_cast<ClickToolChain*>(tc)->gnutriplet());
                QString commTemplate = QStringLiteral("./%S scope %1 %C")
                        .arg(manifest.name()+QStringLiteral("_")+hook.appId); //tell our script the appid

                if(!UbuntuProjectHelper::injectScopeDebugHelper(iniFilePath, debScript, commTemplate, defaultSubCmd))
                    qWarning()<<"Could not write the updated ini file";

                //copy the helper script to the click package tree
                if(QFile::exists(debSourcePath))
                    QFile::copy(debSourcePath,debTargetPath);

            } else if(!hook.desktopFile.isEmpty() && hook.scope.isEmpty()){

                const QString debTargetPath = Utils::FileName::fromString(m_deployDir)
                        .appendPath(debScript).toString();

                //make sure there is no old script in case we don't want to package it
                if(QFile::exists(debTargetPath))
                    QFile::remove(debTargetPath);

                if(!injectDebugScript)
                    continue;

                //copy the helper script to the click package tree
                if(QFile::exists(debSourcePath))
                    QFile::copy(debSourcePath,debTargetPath);

                //inject the debug helper into the Exec line in the desktop file
                //@BUG if there are env vars set in the Exec line (Var=something command) this will fail

                QFile deskFileFd(Utils::FileName::fromString(m_deployDir)
                                 .appendPath(hook.desktopFile)
                                 .toString());

                if(!deskFileFd.open(QIODevice::ReadOnly))
                    continue;

                QString contents;
                {
                    QTextStream deskInStream(&deskFileFd);
                    contents = deskInStream.readAll();
                }
                deskFileFd.close();

                QRegularExpressionMatch m = deskExecRegex.match(contents);
                if(!m.hasMatch())
                    continue;

                QString exec = m.captured(1);
                int idxOfEq = exec.indexOf(QStringLiteral("="));
                exec.remove(0,idxOfEq+1);

                //replaces the exec line with out patched version
                contents.replace(m.capturedStart(1),
                                 m.capturedLength(1),
                                 QStringLiteral("Exec=./%1 app \"%2\"").arg(debScript).arg(exec));

                if(!deskFileFd.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    continue;
                }

                QTextStream outStream(&deskFileFd);
                outStream << contents;
                deskFileFd.close();
            } else {
                qWarning()<<"Ambiguous configuration for hook "<<hook.appId<<" either scope or desktop property has to be set.";
                continue;
            }

            //if(buildConfiguration()->buildType() == ProjectExplorer::BuildConfiguration::Debug) {
                //enable debugging in the apparmor file, this will inject the debug policy
                if (!appArmor.enableDebugging())
                    qWarning() <<"Could not inject debug policy, debugging with gdb will not work";
            //}

            appArmor.save();
        }
    }

    QTimer::singleShot(0,this,SLOT(doNextStep()));
}

void UbuntuPackageStep::doNextStep()
{
    switch (m_state) {
        case Idle: {
            m_futureInterface->setProgressValueAndText(0,tr("Make install"));

            switch (m_packageMode) {
                case OnlyClickBuild:
                    m_state = PreparePackage;
                    doNextStep();
                    return;
                case Default:
                case OnlyMakeInstall:
                    m_state = MakeInstall;

                    if (m_cleanDeployDirectory &&
                            //paranoid double check
                            m_deployDir.endsWith(QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))) {
                        //make sure we always use a clean deploy dir
                        QDir deplDir(m_deployDir);
                        if(deplDir.exists())
                            deplDir.removeRecursively();
                    }

                    setupAndStartProcess(m_MakeParam);
                    break;
            }
            break;
        }
        case MakeInstall: {
            if (!processFinished())
                return;

            if (m_packageMode == OnlyMakeInstall) {

                if(m_futureInterface)
                    reportRunResult(*m_futureInterface, true);
                cleanup();

                return;
            }

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

            QRegularExpression exp((QLatin1String(Constants::UBUNTU_CLICK_SUCCESS_PACKAGE_REGEX)));
            QRegularExpressionMatch m = exp.match(m_lastLine);
            if(m.hasMatch()) {
                m_clickPackageName = m.captured(1);
                emit addOutput(tr("The click package has been created in %1").arg(clickWorkingDir()) ,
                               ProjectExplorer::BuildStep::MessageOutput);
            }

            m_futureInterface->setProgressValueAndText(3,tr("Reviewing click package"));
            m_state = ClickReview;

            m_ReviewParam.setArguments(QString::fromLatin1(Constants::CLICK_REVIEWERSTOOLS_ARGS).arg(packagePath()));
            m_ReviewParam.resolveAll();
            setupAndStartProcess(m_ReviewParam);
            break;
        }
        case ClickReview: {
            //we need to ignore the return code for now,
            //until we have proper support for ignoring specific errors
            if (!processFinished(IgnoreReturnCode))
                return;

            if(m_futureInterface)
                reportRunResult(*m_futureInterface, true);
            cleanup();
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
    if(m_futureInterface)
        reportRunResult(*m_futureInterface, false);

    ProjectExplorer::ProcessParameters *params;
    if (m_state == MakeInstall)
        params = &m_MakeParam;
    else
        params = &m_ClickParam;

    emit addOutput(tr("Could not start process \"%1\" %2")
                   .arg(QDir::toNativeSeparators(params->effectiveCommand()),
                        params->prettyArguments()),
                   BuildStep::ErrorMessageOutput);

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
bool UbuntuPackageStep::cleanDeployDirectory() const
{
    return m_cleanDeployDirectory;
}

void UbuntuPackageStep::setCleanDeployDirectory(bool cleanDeployDirectory)
{
    m_cleanDeployDirectory = cleanDeployDirectory;
}

bool UbuntuPackageStep::treatClickErrorsAsWarnings() const
{
    return m_treatClickErrorsAsWarnings;
}

void UbuntuPackageStep::setTreatClickErrorsAsWarnings(bool arg)
{
    if (m_treatClickErrorsAsWarnings != arg) {
        m_treatClickErrorsAsWarnings = arg;
        emit treatClickErrorsAsWarningsChanged(arg);
    }
}

ProjectExplorer::BuildConfiguration *UbuntuPackageStep::referenceBuildConfig() const
{
    if(m_referenceBuildConfig)
        return m_referenceBuildConfig.data();

    return target()->activeBuildConfiguration();
}

void UbuntuPackageStep::setReferenceBuildConfig(ProjectExplorer::BuildConfiguration *referenceBuildConfig)
{
    m_referenceBuildConfig = referenceBuildConfig;
}

QString UbuntuPackageStep::overrideClickWorkingDir() const
{
    return m_overrideClickWorkingDir;
}

void UbuntuPackageStep::setOverrideClickWorkingDir(const QString &overrideClickWorkingDir)
{
    m_overrideClickWorkingDir = overrideClickWorkingDir;
}

QString UbuntuPackageStep::clickWorkingDir() const
{
    if (!m_overrideClickWorkingDir.isEmpty())
        return m_overrideClickWorkingDir;
    return m_buildDir;
}

QString UbuntuPackageStep::overrideInstallDir() const
{
    return m_overrideDeployDir;
}

void UbuntuPackageStep::setOverrideDeployDir(const QString &overrideInstallDir)
{
    m_overrideDeployDir = overrideInstallDir;
}


UbuntuPackageStep::PackageMode UbuntuPackageStep::packageMode() const
{
    return m_packageMode;
}

void UbuntuPackageStep::setPackageMode(const UbuntuPackageStep::PackageMode &packageMode)
{
    m_packageMode = packageMode;
}


UbuntuPackageStepConfigWidget::UbuntuPackageStepConfigWidget(UbuntuPackageStep *step) :
    SimpleBuildStepConfigWidget(step),
    ui(new Ui::UbuntuPackageStepConfigWidget),
    m_isUpdating(false)
{
    ui->setupUi(this);
    ui->comboBoxMode->addItem(tr("Yes") ,static_cast<int>(UbuntuPackageStep::EnableDebugScript));
    ui->comboBoxMode->addItem(tr("No")  ,static_cast<int>(UbuntuPackageStep::DisableDebugScript));

    connect(step,SIGNAL(packageModeChanged(DebugMode)),this,SLOT(updateMode()));
    connect(step,SIGNAL(treatClickErrorsAsWarningsChanged(bool)),this,SLOT(updateMode()));

    connect(ui->comboBoxMode,SIGNAL(activated(int)),this,SLOT(onModeSelected(int)));
    connect(ui->checkBox,SIGNAL(clicked(bool)), this, SLOT(onClickErrorsToggled(bool)));

    updateMode();
}

UbuntuPackageStepConfigWidget::~UbuntuPackageStepConfigWidget()
{
    delete ui;
}

bool UbuntuPackageStepConfigWidget::showWidget() const
{
    return true;
}

void UbuntuPackageStepConfigWidget::updateMode()
{
    if (m_isUpdating)
        return;

    UbuntuPackageStep *myStep = static_cast<UbuntuPackageStep*>(step());
    int mode = myStep->debugMode();

    m_isUpdating = true;
    int idx = ui->comboBoxMode->findData(mode);
    if(idx >= 0)
        ui->comboBoxMode->setCurrentIndex(idx);

    ui->checkBox->setChecked(myStep->treatClickErrorsAsWarnings());

    m_isUpdating = false;
}

void UbuntuPackageStepConfigWidget::onModeSelected(const int index)
{
    if (m_isUpdating)
        return;

    int mode = ui->comboBoxMode->itemData(index).toInt();
    if ( mode >= UbuntuPackageStep::AutoEnableDebugScript && mode <= UbuntuPackageStep::DisableDebugScript ) {
        m_isUpdating = true;
        static_cast<UbuntuPackageStep*>(step())->setDebugMode(static_cast<UbuntuPackageStep::DebugMode>(mode));
        m_isUpdating = false;
    }
}

void UbuntuPackageStepConfigWidget::onClickErrorsToggled(const bool checked)
{
    if (m_isUpdating)
        return;

    m_isUpdating = true;
    static_cast<UbuntuPackageStep*>(step())->setTreatClickErrorsAsWarnings(checked);
    m_isUpdating = false;
}

} // namespace Internal
} // namespace Ubuntu
