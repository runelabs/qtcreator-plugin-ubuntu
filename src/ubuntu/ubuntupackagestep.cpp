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
    m_packageMode(AutoEnableDebugScript)
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
    return target()->activeBuildConfiguration()->buildDirectory().toString()
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

    bool ubuntuDevice = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(bc->target()->kit()) == Constants::UBUNTU_DEVICE_TYPE_ID;
    bool injectDebugScript = (m_packageMode == EnableDebugScript) ||
            (m_packageMode == AutoEnableDebugScript && bc->buildType() == ProjectExplorer::BuildConfiguration::Debug);

    //debughelper script name, source and destination path
    const QString debScript = QStringLiteral("qtc_device_debughelper.py");
    const QString debSourcePath = QStringLiteral("%1/%2").arg(Constants::UBUNTU_SCRIPTPATH).arg(debScript);
    const QString debTargetPath = QStringLiteral("%1/%2/%3")
            .arg(bc->buildDirectory().toString())
            .arg(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
            .arg(debScript);

    //make sure there is no old script in case we don't want to package it
    if(QFile::exists(debTargetPath))
        QFile::remove(debTargetPath);

    if( injectDebugScript && ubuntuDevice ) {

        QString projectName = target()->project()->displayName();

        QRegularExpression deskExecRegex(QStringLiteral("^(\\s*[Ee][Xx][Ee][cC]=.*)$"),QRegularExpression::MultilineOption);

        UbuntuClickManifest manifest;
        manifest.load(bc->buildDirectory()
                      .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                      .appendPath(QStringLiteral("manifest.json"))
                      .toString(),
                      projectName);

        QList<UbuntuClickManifest::Hook> hooks = manifest.hooks();
        foreach ( const UbuntuClickManifest::Hook &hook, hooks) {

            UbuntuClickManifest appArmor;
            if (!appArmor.load(bc->buildDirectory()
                               .appendPath(QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR))
                               .appendPath(hook.appArmorFile)
                               .toString(),
                               projectName)) {
                qWarning()<<"Could not open the apparmor file for: "<<hook.appId;
                continue;
            }

            if (hook.desktopFile.isEmpty() && !hook.scope.isEmpty()) {
                //this is a scope hook
                qWarning()<<"Packaging Scope for debug is not support yet: "<<hook.appId;
                continue;
            } else if(!hook.desktopFile.isEmpty() && hook.scope.isEmpty()){

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
                                 QStringLiteral("Exec=./%1 \"%2\"").arg(debScript).arg(exec));


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

            //enable debugging in the apparmor file, this will inject the debug policy
            if (!appArmor.enableDebugging())
                qWarning() <<"Could not inject debug policy, debugging with gdb will not work";

            appArmor.save();
        }

        //copy the helper script to the click package tree
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

UbuntuPackageStepConfigWidget::UbuntuPackageStepConfigWidget(UbuntuPackageStep *step) :
    SimpleBuildStepConfigWidget(step),
    ui(new Ui::UbuntuPackageStepConfigWidget),
    m_isUpdating(false)
{
    ui->setupUi(this);
    ui->comboBoxMode->addItem(tr("Auto"),static_cast<int>(UbuntuPackageStep::AutoEnableDebugScript));
    ui->comboBoxMode->addItem(tr("Yes") ,static_cast<int>(UbuntuPackageStep::EnableDebugScript));
    ui->comboBoxMode->addItem(tr("No")  ,static_cast<int>(UbuntuPackageStep::DisableDebugScript));
    connect(step,SIGNAL(packageModeChanged(PackageMode)),this,SLOT(updateMode()));
    connect(ui->comboBoxMode,SIGNAL(activated(int)),this,SLOT(onModeSelected(int)));
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
