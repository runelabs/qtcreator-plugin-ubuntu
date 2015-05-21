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

#include "ubuntulocalrunconfiguration.h"
#include "ubuntuproject.h"
#include "ubuntuprojecthelper.h"
#include "ubuntuclickmanifest.h"
#include "ubunturemoterunconfiguration.h"
#include "ubuntucmakecache.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
using namespace Ubuntu::Internal;

enum {
    debug = 1
};

UbuntuLocalRunConfiguration::UbuntuLocalRunConfiguration(ProjectExplorer::Target *parent, Core::Id id)
    : ProjectExplorer::LocalApplicationRunConfiguration(parent, id)
{
    setDisplayName(appId());
    addExtraAspect(new ProjectExplorer::LocalEnvironmentAspect(this));
}

UbuntuLocalRunConfiguration::UbuntuLocalRunConfiguration(ProjectExplorer::Target *parent, UbuntuLocalRunConfiguration *source)
    : ProjectExplorer::LocalApplicationRunConfiguration(parent,source)
{
}

QWidget *UbuntuLocalRunConfiguration::createConfigurationWidget()
{
    return NULL;
}

bool UbuntuLocalRunConfiguration::isEnabled() const
{
    return true;
}

QString UbuntuLocalRunConfiguration::appId() const
{
    if(id().toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_APP_ID)))
        return id().suffixAfter(Constants::UBUNTUPROJECT_RUNCONTROL_APP_ID);
    else
        return id().suffixAfter(Constants::UBUNTUPROJECT_RUNCONTROL_SCOPE_ID);
}

QString UbuntuLocalRunConfiguration::executable() const
{
    return m_executable;
}

QString UbuntuLocalRunConfiguration::workingDirectory() const
{
    return m_workingDir.toString();
}

QString UbuntuLocalRunConfiguration::commandLineArguments() const
{
    return Utils::QtcProcess::joinArgs(m_args);
}

ProjectExplorer::ApplicationLauncher::Mode UbuntuLocalRunConfiguration::runMode() const
{
    return ProjectExplorer::ApplicationLauncher::Gui;
}

ProjectExplorer::RunConfiguration::ConfigurationState UbuntuLocalRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if(target()->project()->id() != Constants::UBUNTUPROJECT_ID) {
        QString idString = id().toString();
        if(idString.startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_APP_ID))) {
            if (ensureClickAppConfigured(errorMessage))
                return Configured;
            else
                return UnConfigured;
        }
        else if(idString.startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_SCOPE_ID))) {
            if (ensureScopesAppConfigured(errorMessage))
                return Configured;
            else
                return UnConfigured;
        }

        //all other hook types can not be configured
        //should never happen
        if(errorMessage)
            *errorMessage = tr("Unknown hook type, only scope and app hooks are supported");
        return UnConfigured;
    }

    if (ensureUbuntuProjectConfigured(errorMessage))
        return Configured;

    return UnConfigured;
}

QString UbuntuLocalRunConfiguration::getDesktopFile(ProjectExplorer::RunConfiguration* config, QString appId, QString *errorMessage)
{

    //lambda reads the desktop file from the manifest
    auto getDesktopFromManifest = [&]( UbuntuClickManifest &manifest ){
        QList<UbuntuClickManifest::Hook> hooks = manifest.hooks();
        if (hooks.isEmpty()) {
            if(errorMessage)
                *errorMessage = tr("No valid hooks property in the manifest");

            return QString();
        }

        foreach(const UbuntuClickManifest::Hook& hook, hooks) {
            if( hook.appId == appId )
                return hook.desktopFile;
        }

        return QString();
    };

    QString manifestPath;
    if(qobject_cast<UbuntuLocalRunConfiguration*>(config)) {
        //For a locally compiled project, we have no CLICK_MODE enabled, that means we have to search
        //the project and build trees for our desktop file so we can query the main qml file from the desktop file

        Utils::FileName projectDir = config->target()->project()->projectDirectory();

        //lambda that searches for the desktop file in the project and build directory
        auto searchDesktopFile = [&]( const QString &desktopFileName ){
            QRegularExpression desktopFinder = QRegularExpression(QString::fromLatin1("^%1$")
                                                                  .arg(desktopFileName));

            QFileInfo desktopInfo = UbuntuProjectHelper::findFileRecursive(config->target()->activeBuildConfiguration()->buildDirectory(),
                                                                            desktopFinder).toFileInfo();
            if (!desktopInfo.exists()) {

                //search again in the project directory
                desktopInfo = UbuntuProjectHelper::findFileRecursive(config->target()->project()->projectDirectory(),
                                                                      desktopFinder).toFileInfo();

                if(!desktopInfo.exists())
                    return QString();
            }
            return desktopInfo.absoluteFilePath();
        };

        //first lets check if the informations in the manifest file are helpful
        QString manifestPath = UbuntuProjectHelper::getManifestPath(config->target(),QStringLiteral("manifest.json"));

        //make the path relative to the project dir
        QDir projectDirDir(projectDir.toString());
        manifestPath = projectDirDir.relativeFilePath(manifestPath);

        //search for the manifest file in the project dir AND in the builddir
        QList<Utils::FileName> searchPaths{
            projectDir.appendPath(manifestPath),
            config->target()->activeBuildConfiguration()->buildDirectory()
                    .appendPath(QFileInfo(manifestPath).path())
                    .appendPath(QStringLiteral("manifest.json"))
        };

        for(Utils::FileName path : searchPaths) {
            QFileInfo manifestFile(path.toString());
            if(debug) qDebug()<<"Searching for the manifest file: "<<manifestFile.absoluteFilePath();
            if ( manifestFile.exists() ) {
                UbuntuClickManifest manifest;
                if(manifest.load(manifestFile.absoluteFilePath())){
                    QString desktop = getDesktopFromManifest(manifest);

                    if(!desktop.isEmpty()) {
                        QFileInfo d(desktop);

                        desktop = searchDesktopFile(d.fileName());
                        if(!desktop.isEmpty())
                            return desktop;
                    }
                }
            }
        }

        //ok we had no luck in the manifest file, lets search for a desktop file that is named like the appid
        QString desk = searchDesktopFile(appId.append(QStringLiteral(".desktop")));
        if ( !desk.isEmpty() )
            return desk;

        //still nothing, last hope search for a desktop file with the project name
        //@FIXME THIS WILL NOT WORK CORRECTLY WITH MULTIPLE HOOKS IN THE SAME PROJECT
        desk = searchDesktopFile(config->target()->project()->displayName().append(QStringLiteral(".desktop")));
        if(desk.isEmpty()) {
            if(errorMessage)
                *errorMessage = tr("Could not find a desktop file for the hook: %1, \nmaybe it is missing in the install targets.").arg(appId);

            return QString();
        }

        return desk;
    }


     UbuntuRemoteRunConfiguration *remConf = qobject_cast<UbuntuRemoteRunConfiguration*>(config);
    if (!remConf) {
        if(errorMessage)
            *errorMessage = tr("Invalid configuration type");
        return QString();
    }

    QDir package_dir(remConf->packageDir());
    if(!package_dir.exists()) {
        if(errorMessage)
            *errorMessage = tr("No packaging directory available, please check if the deploy configuration is correct.");

        return QString();
    }
    manifestPath = package_dir.absoluteFilePath(QStringLiteral("manifest.json"));

    //read the manifest
    UbuntuClickManifest manifest;
    QString manifestErrMsg;
    if(!manifest.load(manifestPath,nullptr,&manifestErrMsg)) {
        if(errorMessage)
            *errorMessage = tr("Could not open the manifest file in the package directory: %1.").arg(manifestErrMsg);

        return QString();
    }

    QString desk = getDesktopFromManifest(manifest);
    if(!desk.isEmpty())
        return QDir::cleanPath(package_dir.absoluteFilePath(desk));

    if (errorMessage)
        *errorMessage = tr("Could not find a %1 hook in the manifest file").arg(appId);

    return QString();
}

bool UbuntuLocalRunConfiguration::readDesktopFile(const QString &desktopFile, QString *executable, QStringList *arguments, QString *errorMessage)
{
    QFile desktop(desktopFile);
    if(!desktop.exists()) {
        if(errorMessage)
            *errorMessage = tr("Desktop file does not exist");
        return false;
    }
    if(!desktop.open(QIODevice::ReadOnly)) {
        if(errorMessage)
            *errorMessage = tr("Could not open desktop file for reading");
        return false;
    }

    QString execLine;
    QString name;

    QTextStream in(&desktop);
    while(!in.atEnd()) {
        QString line = in.readLine();
        if(line.startsWith(QLatin1String("#")))
            continue;

        line = line.mid(0,line.indexOf(QChar::fromLatin1('#'))).trimmed();
        if(line.startsWith(QLatin1String("Exec"),Qt::CaseInsensitive)) {
            execLine = line.mid(line.indexOf(QChar::fromLatin1('='))+1);
            if(debug) qDebug()<<"Found exec line: "<<execLine;
            continue;
        } else if(line.startsWith(QLatin1String("Name"),Qt::CaseInsensitive)) {
            name = line.mid(line.indexOf(QChar::fromLatin1('='))+1);
            if(debug) qDebug()<<"Found name line: "<<name;
            continue;
        }
    }

    QStringList args = Utils::QtcProcess::splitArgs(execLine);
    *executable = args.takeFirst();

    //if debugging is enabled we inject the debughelper, so we need
    //to remove it and the mode argument here
    if(executable->contains(QStringLiteral("qtc_device_debughelper.py"))) {
        args = Utils::QtcProcess::splitArgs(args[1]);
        *executable = args.takeFirst();
    }

    *arguments  = args;
    qDebug()<<args;

    return true;
}

bool UbuntuLocalRunConfiguration::ensureClickAppConfigured(QString *errorMessage)
{
    QString desktopFile = getDesktopFile(this,appId(),errorMessage);
    if(desktopFile.isEmpty())
        return false;

    /*
     * Tries to read the Exec line from the desktop file, to
     * extract arguments and to know which "executor" is used
     */
    QStringList args;
    QString command;

    if(!UbuntuLocalRunConfiguration::readDesktopFile(desktopFile,&command,&args,errorMessage))
        return false;

    m_workingDir = target()->activeBuildConfiguration()->buildDirectory();

    QFileInfo commInfo(command);
    if(commInfo.fileName().startsWith(QLatin1String("qmlscene"))) {
        m_executable = QtSupport::QtKitInformation::qtVersion(target()->kit())->qmlsceneCommand();

        QString mainQml;
        foreach(const QString &arg, args) {
            if(arg.contains(QLatin1String(".qml"))) {
                QFileInfo info(arg);
                mainQml = info.fileName();
            }
        }

        if(mainQml.isEmpty()) {
            if(errorMessage)
                *errorMessage = tr("Could not get the main qml file from the desktop EXEC line.");
            return false;
        }

        QFileInfo mainFileInfo = UbuntuProjectHelper::findFileRecursive(target()->project()->projectDirectory(),
                                                                         QString::fromLatin1("^%1$").arg(mainQml)).toFileInfo();

        if(!mainFileInfo.exists()) {
            if(errorMessage)
                *errorMessage = tr("Could not find the main qml file (%1) in the project directory.").arg(mainQml);
            return false;
        }

        m_args = QStringList()<<mainFileInfo.absoluteFilePath();
    } else if(commInfo.completeBaseName().startsWith(QLatin1String(Ubuntu::Constants::UBUNTUHTML_PROJECT_LAUNCHER_EXE))) {
        if(errorMessage)
            *errorMessage = tr("There is no local run support for html click apps using cmake yet.");
        return false;
        /*
        Utils::Environment env = Utils::Environment::systemEnvironment();
        m_executable = env.searchInPath(QString::fromLatin1(Ubuntu::Constants::UBUNTUHTML_PROJECT_LAUNCHER_EXE));
        m_args = args;
        */
    } else {
        //looks like a application without a launcher
        if(target()->project()->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID) {
            QmakeProjectManager::QmakeProject* pro = static_cast<QmakeProjectManager::QmakeProject*> (target()->project());
            foreach(const QmakeProjectManager::QmakeProFileNode* applPro, pro->applicationProFiles()) {
                QmakeProjectManager::TargetInformation info = applPro->targetInformation();
                if(applPro->targetInformation().valid) {
                    if(info.target == commInfo.fileName()) {
                        m_executable = info.buildDir + QDir::separator() + info.target;
                        break;
                    }
                }
            }
        } else {
            CMakeProjectManager::CMakeProject* proj = static_cast<CMakeProjectManager::CMakeProject*> (target()->project());
            QList<CMakeProjectManager::CMakeBuildTarget> targets = proj->buildTargets();
            foreach (const CMakeProjectManager::CMakeBuildTarget& t, targets) {
                if(t.targetType != CMakeProjectManager::ExecutableType)
                    continue;

                QFileInfo targetInfo(t.executable);
                if(!targetInfo.exists())
                    continue;

                if(targetInfo.fileName() == commInfo.fileName()) {
                    m_executable = targetInfo.absoluteFilePath();
                    break;
                }
            }
        }

        if (m_executable.isEmpty()) {
            if(errorMessage)
                *errorMessage = tr("Could not find executable specified in the desktop file");
            return false;
        }
        m_args = args;
        m_workingDir = target()->project()->projectDirectory();
    }
    if(debug) qDebug()<<"Configured with: "<<m_executable<<" "<<m_args.join(QChar::fromLatin1(' '));
    return true;
}

bool UbuntuLocalRunConfiguration::ensureScopesAppConfigured(QString *errorMessage)
{
    m_workingDir = target()->activeBuildConfiguration()->buildDirectory();

    Utils::Environment env = Utils::Environment::systemEnvironment();
    m_executable = env.searchInPath(QString::fromLatin1(Ubuntu::Constants::UBUNTUSCOPES_PROJECT_LAUNCHER_EXE)).toString();

    if(m_executable.isEmpty()) {
        if(errorMessage)
            *errorMessage = tr("Could not find launcher %1 in path").arg(QLatin1String(Ubuntu::Constants::UBUNTUSCOPES_PROJECT_LAUNCHER_EXE));
        return false;
    }

    Utils::FileName iniFile = UbuntuProjectHelper::findScopesIniRecursive(target()->activeBuildConfiguration()->buildDirectory(),appId());
    if(iniFile.toFileInfo().exists())
        m_args = QStringList()<<QString(iniFile.toFileInfo().absoluteFilePath());

    return true;
}

bool UbuntuLocalRunConfiguration::ensureUbuntuProjectConfigured(QString *errorMessage)
{
    UbuntuProject* ubuntuProject = qobject_cast<UbuntuProject*>(target()->project());
    if (ubuntuProject) {
        m_workingDir = ubuntuProject->projectDirectory();
        if (ubuntuProject->mainFile().compare(QString::fromLatin1("www/index.html"), Qt::CaseInsensitive) == 0) {
            Utils::Environment env = Utils::Environment::systemEnvironment();
            m_executable = env.searchInPath(QString::fromLatin1(Ubuntu::Constants::UBUNTUHTML_PROJECT_LAUNCHER_EXE)).toString();
            m_args = QStringList()<<QString::fromLatin1("--www=%0/www").arg(ubuntuProject->projectDirectory().toString())
                                  <<QString::fromLatin1("--inspector");
        } else if (ubuntuProject->mainFile().endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive)) {
            Utils::Environment env = Utils::Environment::systemEnvironment();

            if(!readDesktopFile(ubuntuProject->projectDirectory()
                                .appendPath(ubuntuProject->mainFile())
                                .toString()
                                ,&m_executable,&m_args,errorMessage))
                return false;

            m_executable = env.searchInPath(QString::fromLatin1(Ubuntu::Constants::UBUNTUWEBAPP_PROJECT_LAUNCHER_EXE)).toString();
        } else {
            m_executable = QtSupport::QtKitInformation::qtVersion(target()->kit())->qmlsceneCommand();
            m_args = QStringList()<<QString(QLatin1String("%0.qml")).arg(ubuntuProject->displayName());
        }

        if(m_executable.isEmpty()) {
            if(errorMessage)
                *errorMessage = tr("Could not find a launcher for this projecttype in path");
            return false;
        }
        return true;
    }
    if(errorMessage)
        *errorMessage = tr("Unsupported Project Type used with UbuntuRunConfiguration");

    return false;
}


void UbuntuLocalRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    QSet<QString> usedPaths;

    //lambda checks if the executable is in a qmldir and add its to QML2_IMPORT_PATH if
    //required
    auto loc_addToImportPath = [&usedPaths,&env] (const QString &loc_executable) {
        QDir d = QFileInfo(loc_executable).absoluteDir();
        if(debug) qDebug()<<"Looking in the dir: "<<d.absolutePath()<<loc_executable;
        if(d.exists(QLatin1String("qmldir"))) {
            QString path = QDir::cleanPath(d.absolutePath()+QDir::separator()+QLatin1String(".."));
            if(usedPaths.contains(path))
                return;

            if(debug) qDebug()<<"Adding"<<path<<"to QML2_IMPORT_PATH";
            env.appendOrSet(QStringLiteral("QML2_IMPORT_PATH"),path,QStringLiteral(":"));
            usedPaths.insert(path);
        }
    };

    if(id().toString().startsWith(QLatin1String(Constants::UBUNTUPROJECT_RUNCONTROL_APP_ID))) {
        if(target()->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID) {
            CMakeProjectManager::CMakeProject* proj = static_cast<CMakeProjectManager::CMakeProject*> (target()->project());
            QList<CMakeProjectManager::CMakeBuildTarget> targets = proj->buildTargets();
            foreach (const CMakeProjectManager::CMakeBuildTarget& t, targets) {
                if(t.targetType == CMakeProjectManager::DynamicLibraryType)
                    loc_addToImportPath(t.executable);
            }
        } else if (target()->project()->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID) {
            QmakeProjectManager::QmakeProject* pro = static_cast<QmakeProjectManager::QmakeProject*> (target()->project());
            foreach(const QmakeProjectManager::QmakeProFileNode* applPro, pro->allProFiles()) {
                if(applPro->projectType() != QmakeProjectManager::ApplicationTemplate &&
                        applPro->projectType() != QmakeProjectManager::SharedLibraryTemplate) {
                    continue;
                }

                QmakeProjectManager::TargetInformation info = applPro->targetInformation();
                if(applPro->targetInformation().valid)
                    loc_addToImportPath(info.buildDir + QDir::separator() + info.target);
            }
        }
    }
}

bool UbuntuLocalRunConfiguration::isConfigured() const
{
    return false;
}

