#include "ubuntupackagestep.h"
#include "ui_ubuntupackagestepconfigwidget.h"
#include "ubuntuconstants.h"
#include "clicktoolchain.h"
#include "ubuntuprojectguesser.h"
#include "ubuntuclickmanifest.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <utils/qtcprocess.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <QDir>
#include <QTimer>
#include <QRegularExpression>
#include <QVariant>
#include <glib-2.0/glib.h>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

const char PACKAGE_MODE_KEY[] = "Ubuntu.UbuntuPackageStep.PackageMode";

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
    m_packageMode(EnableDebugScript)
{
    setDefaultDisplayName(tr("UbuntuSDK Click build"));
}

UbuntuPackageStep::UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl, UbuntuPackageStep *other) :
    ProjectExplorer::BuildStep(bsl,other),
    m_state(Idle),
    m_futureInterface(0),
    m_process(0),
    m_outputParserChain(0),
    m_packageMode(other->m_packageMode)
{

}

UbuntuPackageStep::~UbuntuPackageStep()
{
    cleanup();
}

bool UbuntuPackageStep::init()
{
    m_tasks.clear();

    QString projectDir = target()->project()->projectDirectory();
    QString deployDir;
    Utils::Environment env = Utils::Environment::systemEnvironment();
    Utils::AbstractMacroExpander *mExp = 0;

    {
        ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();
        if(bc) {
            m_buildDir  = bc->buildDirectory().toString();
            deployDir = bc->buildDirectory().toString()
                    + QDir::separator()
                    + QString::fromLatin1(Constants::UBUNTU_DEPLOY_DESTDIR);
            env = bc->environment();
            mExp = bc->macroExpander();
        } else {
            //cmake projects NEED a buildconfiguration
            if (target()->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID) {
                ProjectExplorer::Task t(ProjectExplorer::Task::Error
                                        ,tr("No valid BuildConfiguration set for step: %1").arg(displayName())
                                        ,Utils::FileName(),-1
                                        ,ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
                m_tasks.append(t);

                //UbuntuClickPackageStep::run will stop if tasks exist
                return true;
            } else {
                //backward compatibility, old HTML5 projects did not have a Buildconfiguration
                //this would crash otherwise

                //ubuntu + qml project types
                QDir pDir(projectDir);
                m_buildDir = QDir::cleanPath(target()->project()->projectDirectory()
                                             +QDir::separator()+QStringLiteral("..")
                                             +QDir::separator()+pDir.dirName()+QStringLiteral("_build"));
                deployDir = m_buildDir+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR);

                //clean up the old "build"
                QDir bd(m_buildDir);
                if(!bd.exists(m_buildDir))
                    bd.mkdir(m_buildDir);
            }
        }
    }

    //build the make process arguments
    {
        if (target()->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID) {
            QStringList arguments;
            arguments << QStringLiteral("DESTDIR=%1").arg(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                      << QStringLiteral("install");

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
                      << QStringLiteral("--delete")
                      << QStringLiteral("--exclude")<<QStringLiteral(".bzr")
                      << QStringLiteral("--exclude")<<QStringLiteral(".git")
                      << QStringLiteral("--exclude")<<QStringLiteral(".hg")
                      << QStringLiteral("--exclude")<<QStringLiteral(".svn")
                      << QStringLiteral("--exclude")<<QStringLiteral("*.qmlproject")
                      << QStringLiteral("--exclude")<<QStringLiteral("*.user")
                      << QStringLiteral("--exclude")<<QStringLiteral("tests")
                      << QStringLiteral("--exclude")<<QStringLiteral("Makefile")
                      << QStringLiteral("--exclude")<<QStringLiteral(".excludes")
                      << QStringLiteral("--exclude")<<QStringLiteral("*.ubuntuhtmlproject");

            QFile excludes (projectDir+QDir::separator()+QStringLiteral(".excludes"));
            if (excludes.open(QIODevice::ReadOnly)) {
                while (excludes.canReadLine()) {
                    arguments << QStringLiteral("--exclude") << QString::fromUtf8(excludes.readLine());
                }
                excludes.close();
            }

            arguments << projectDir+QDir::separator()
                      << deployDir;

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
        arguments << QLatin1String("build")
                  << deployDir;

        ProjectExplorer::ProcessParameters* params = &m_ClickParam;
        params->setMacroExpander(mExp);

        //setup process parameters
        params->setWorkingDirectory(m_buildDir);
        params->setCommand(QLatin1String("click"));
        params->setArguments(Utils::QtcProcess::joinArgs(arguments));

        Utils::Environment tmpEnv = env;
        // Force output to english for the parsers. Do this here and not in the toolchain's
        // addToEnvironment() to not screw up the users run environment.
        tmpEnv.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        params->setEnvironment(tmpEnv);

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


    setPackageMode(AutoEnableDebugScript);
    if (map.contains(QLatin1String(PACKAGE_MODE_KEY))) {
        int mode = map[QLatin1String(PACKAGE_MODE_KEY)].toInt();

        //AutoEnableDebugScript is deprecated, we always want the helper packaged
        if(mode == AutoEnableDebugScript)
            mode = EnableDebugScript;

        if(mode >= AutoEnableDebugScript && mode <= DisableDebugScript)
            setPackageMode(static_cast<PackageMode>(mode));
    }
    return true;
}

QVariantMap UbuntuPackageStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    if(map.isEmpty())
        return map;

    map.insert(QLatin1String(PACKAGE_MODE_KEY),static_cast<int>(m_packageMode));
    return map;
}

QString UbuntuPackageStep::packagePath() const
{
    if(m_clickPackageName.isEmpty())
        return QString();
    return m_buildDir
            + QDir::separator()
            + m_clickPackageName;
}

UbuntuPackageStep::PackageMode UbuntuPackageStep::packageMode() const
{
    return m_packageMode;
}

void UbuntuPackageStep::setPackageMode(UbuntuPackageStep::PackageMode arg)
{
    if (m_packageMode != arg) {
        m_packageMode = arg;
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

    if(debug) qDebug()<<"Starting process "<<params.effectiveCommand()<<params.effectiveArguments();

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

    if(!bc) {
        QTimer::singleShot(0,this,SLOT(doNextStep()));
        return;
    }

    bool ubuntuDevice = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(bc->target()->kit()).toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID));
    bool injectDebugScript = (m_packageMode == EnableDebugScript || m_packageMode == AutoEnableDebugScript);

    //debughelper script name, source and destination path
    const QString debScript = QStringLiteral("qtc_device_debughelper.py");
    const QString debSourcePath = QStringLiteral("%1/%2").arg(Constants::UBUNTU_SCRIPTPATH).arg(debScript);

    if(ubuntuDevice) {
        QRegularExpression deskExecRegex(QStringLiteral("^(\\s*[Ee][Xx][Ee][cC]=.*)$"),QRegularExpression::MultilineOption);

        UbuntuClickManifest manifest;
        if(!manifest.load(bc->buildDirectory()
                          .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                          .appendPath(QStringLiteral("manifest.json"))
                          .toString())) {

            emit addOutput(tr("Could not find the manifest.json file in %1.\nPlease check if it is added to the install targets in your project file")
                           .arg(bc->buildDirectory()
                                .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                                .toString()),
                           BuildStep::ErrorMessageOutput);

            m_futureInterface->reportResult(false);
            cleanup();
            emit finished();
            return;
        }

        QList<UbuntuClickManifest::Hook> hooks = manifest.hooks();
        foreach ( const UbuntuClickManifest::Hook &hook, hooks) {

            UbuntuClickManifest appArmor;
            if (!appArmor.load(bc->buildDirectory()
                               .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                               .appendPath(hook.appArmorFile)
                               .toString())) {
                qWarning()<<"Could not open the apparmor file for: "<<hook.appId;
                continue;
            }

            if (hook.desktopFile.isEmpty() && !hook.scope.isEmpty()) {
                //this is a scope hook
                const QString iniFilePath(bc->buildDirectory()
                                          .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                                          .appendPath(hook.scope)
                                          .appendPath(manifest.name()+QStringLiteral("_")+hook.appId+QStringLiteral(".ini"))
                                          .toString());

                const QString debTargetPath = bc->buildDirectory()
                        .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                        .appendPath(hook.scope)
                        .appendPath(debScript).toString();

                //make sure there is no old script in case we don't want to package it
                if(QFile::exists(debTargetPath))
                    QFile::remove(debTargetPath);

                if(!QFile::exists(iniFilePath))
                    continue;

                if(!injectDebugScript)
                    continue;

                GKeyFile* keyFile = g_key_file_new();
                GKeyFileFlags flags = static_cast<GKeyFileFlags>(G_KEY_FILE_KEEP_TRANSLATIONS|G_KEY_FILE_KEEP_COMMENTS);
                if(!g_key_file_load_from_file(keyFile,qPrintable(iniFilePath),flags,NULL)){
                    g_key_file_free(keyFile);
                    qWarning()<<"Could not read the ini file";
                    continue;
                }

                QString subCmd;
                if(g_key_file_has_key(keyFile,"ScopeConfig","ScopeRunner",NULL)) {
                    gchar *value = g_key_file_get_string(keyFile,"ScopeConfig","ScopeRunner",NULL);
                    if(value == NULL) {
                        qWarning()<<"Could not read the ScopeRunner entry";
                        g_key_file_free(keyFile);
                        continue;
                    }

                    subCmd = QString::fromUtf8(value);
                    g_free(value);

                } else {
                    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
                    if(!tc || tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
                        qWarning()<<"Incompatible Toolchain for hook"<<hook.appId;
                        continue;
                    }

                    subCmd = QStringLiteral("/usr/lib/%1/unity-scopes/scoperunner '' %S ").arg(static_cast<ClickToolChain*>(tc)->gnutriplet());
                }

                QString command = QStringLiteral("./%1 scope %2 %3")
                        .arg(debScript)
                        .arg(manifest.name()+QStringLiteral("_")+hook.appId) //tell our script the appid
                        .arg(subCmd);

                g_key_file_set_string(keyFile,"ScopeConfig","ScopeRunner",command.toUtf8().data());

                //copy the helper script to the click package tree
                if(QFile::exists(debSourcePath))
                    QFile::copy(debSourcePath,debTargetPath);

                g_key_file_set_boolean(keyFile,"ScopeConfig","DebugMode",TRUE);

                gsize size = 0;
                gchar *settingData = g_key_file_to_data (keyFile, &size, NULL);
                if(!settingData) {
                    qWarning()<<"Could not convert the new data into the ini file";
                    g_key_file_free(keyFile);
                    continue;
                }

                gboolean ret = g_file_set_contents (qPrintable(iniFilePath), settingData, size,  NULL);
                g_free (settingData);
                g_key_file_free (keyFile);

                if(!ret)
                    qWarning()<<"Could not write the updated ini file";

            } else if(!hook.desktopFile.isEmpty() && hook.scope.isEmpty()){

                const QString debTargetPath = bc->buildDirectory()
                        .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
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

                QFile deskFileFd(bc->buildDirectory()
                                 .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
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

            //if(target()->activeBuildConfiguration()->buildType() == ProjectExplorer::BuildConfiguration::Debug) {
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
                emit addOutput(tr("The click package has been created in %1").arg(m_buildDir) ,
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

UbuntuPackageStepConfigWidget::UbuntuPackageStepConfigWidget(UbuntuPackageStep *step) :
    SimpleBuildStepConfigWidget(step),
    ui(new Ui::UbuntuPackageStepConfigWidget),
    m_isUpdating(false)
{
    ui->setupUi(this);
    ui->comboBoxMode->addItem(tr("Yes") ,static_cast<int>(UbuntuPackageStep::EnableDebugScript));
    ui->comboBoxMode->addItem(tr("No")  ,static_cast<int>(UbuntuPackageStep::DisableDebugScript));
    connect(step,SIGNAL(packageModeChanged(PackageMode)),this,SLOT(updateMode()));
    connect(ui->comboBoxMode,SIGNAL(activated(int)),this,SLOT(onModeSelected(int)));

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

    int mode = static_cast<UbuntuPackageStep*>(step())->packageMode();
    int idx = ui->comboBoxMode->findData(mode);

    if(idx >= 0) {
        m_isUpdating = true;
        ui->comboBoxMode->setCurrentIndex(idx);
        m_isUpdating = false;
    }
}

void UbuntuPackageStepConfigWidget::onModeSelected(const int index)
{
    if (m_isUpdating)
        return;

    int mode = ui->comboBoxMode->itemData(index).toInt();
    if ( mode >= UbuntuPackageStep::AutoEnableDebugScript && mode <= UbuntuPackageStep::DisableDebugScript ) {
        m_isUpdating = true;
        static_cast<UbuntuPackageStep*>(step())->setPackageMode(static_cast<UbuntuPackageStep::PackageMode>(mode));
        m_isUpdating = false;
    }

}

} // namespace Internal
} // namespace Ubuntu
