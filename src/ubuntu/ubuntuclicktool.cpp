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
#include "ubuntuclickmanifest.h"
#include "ubuntuconstants.h"
#include "ubuntushared.h"
#include "clicktoolchain.h"

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
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <utils/qtcprocess.h>
#include <utils/environment.h>
#include <utils/consoleprocess.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <cmakeprojectmanager/cmakebuildconfiguration.h>

#include <QDebug>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};


/**
* Initialize the m_strClickChrootSuffix from the environment variable
*/
QString UbuntuClickTool::m_strClickChrootSuffix = QProcessEnvironment::systemEnvironment().value(QLatin1String(Constants::UBUNTU_CLICK_CHROOT_SUFFIX_ENV_VAR),QLatin1String(Constants::UBUNTU_CLICK_CHROOT_DEFAULT_NAME));

/**
 * @brief UbuntuClickTool::UbuntuClickTool
 * Implements functionality needed for executing the click
 * tool
 */
UbuntuClickTool::UbuntuClickTool()
{
}

/**
 * @brief UbuntuClickTool::clickChrootSuffix
 * Returns the click chroot suffix to be used with click operations
 */
QString UbuntuClickTool::clickChrootSuffix()
{
    return m_strClickChrootSuffix;
}

/**
 * @brief UbuntuClickTool::parametersForCreateChroot
 * Initializes a ProjectExplorer::ProcessParameters object with command and arguments
 * to create a new chroot
 */
void UbuntuClickTool::parametersForCreateChroot(const Target &target, ProjectExplorer::ProcessParameters *params)
{
    QString command = QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_CREATE_ARGS)
            .arg(Constants::UBUNTU_SCRIPTPATH)
            .arg(target.architecture)
            .arg(target.framework)
            .arg(target.series)
            .arg(clickChrootSuffix());
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
                    .arg(target.framework)
                    .arg(target.series)
                    .arg(clickChrootSuffix());
            break;
        case Delete:
            params->setCommand(QLatin1String(Constants::UBUNTU_SUDO_BINARY));
            arguments = QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_DESTROY_ARGS)
                    .arg(Constants::UBUNTU_SCRIPTPATH)
                    .arg(target.architecture)
                    .arg(target.framework)
                    .arg(target.series)
                    .arg(clickChrootSuffix());
            break;
    }


    params->setEnvironment(Utils::Environment::systemEnvironment());
    params->setArguments(arguments);
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

    args << QString(QLatin1String(Constants::UBUNTU_CLICK_OPEN_TERMINAL))
            .arg(target.architecture)
            .arg(target.framework)
            .arg(target.series)
            .arg(clickChrootSuffix());

    if(!QProcess::startDetached(term,args,QDir::homePath())) {
        printToOutputPane(QLatin1String(Constants::UBUNTU_CLICK_OPEN_TERMINAL_ERROR));
    }
}

bool UbuntuClickTool::getTargetFromUser(Target *target, const QString &framework)
{
    QList<UbuntuClickTool::Target> targets = UbuntuClickTool::listAvailableTargets(framework);
    if (!targets.size()) {
        QString message = QCoreApplication::translate("UbuntuClickTool",Constants::UBUNTU_CLICK_NOTARGETS_MESSAGE);
        if(!framework.isEmpty()) {
            message = QCoreApplication::translate("UbuntuClickTool",Constants::UBUNTU_CLICK_NOTARGETS_FRAMEWORK_MESSAGE)
                    .arg(framework);
        }

        QMessageBox::warning(Core::ICore::mainWindow(),
                             QCoreApplication::translate("UbuntuClickTool",Constants::UBUNTU_CLICK_NOTARGETS_TITLE),
                             message);
        return false;
    }

    //if we have only 1 target there is nothing to choose
    if(targets.size() == 1){
        *target = targets[0];
        return true;
    }

    QStringList items;
    foreach(const UbuntuClickTool::Target& t, targets)
        items << QString::fromLatin1("%0-%1").arg(t.framework).arg(t.architecture);

    bool ok = false;
    QString item = QInputDialog::getItem(Core::ICore::mainWindow()
                                         ,QCoreApplication::translate("UbuntuClickTool",Constants::UBUNTU_CLICK_SELECT_TARGET_TITLE)
                                         ,QCoreApplication::translate("UbuntuClickTool",Constants::UBUNTU_CLICK_SELECT_TARGET_LABEL)
                                         ,items,0,false,&ok);
    //get index of item in the targets list
    int idx = items.indexOf(item);
    if(!ok || idx < 0 || idx >= targets.size())
        return false;

    *target = targets[idx];
    return true;
}

QString UbuntuClickTool::targetBasePath(const UbuntuClickTool::Target &target)
{
    return QString::fromLatin1("%1/%2-%3-%4")
            .arg(QLatin1String(Constants::UBUNTU_CLICK_CHROOT_BASEPATH))
	    .arg(clickChrootSuffix())
            .arg(target.framework)
            .arg(target.architecture);
}

/*!
 * \brief UbuntuClickTool::getSupportedFrameworks
 * returns all available frameworks on the host system
 */
QStringList UbuntuClickTool::getSupportedFrameworks(const UbuntuClickTool::Target *target)
{
#if 0
    QProcess proc;
    proc.setProgram(QStringLiteral("click"));

    QStringList args;
    if (target) {
        args << QStringLiteral("chroot")
             << QStringLiteral("-a")
             << target->architecture
             << QStringLiteral("-f")
             << target->framework
             << QStringLiteral("run")
             << QStringLiteral("click");
    }
    args << QStringLiteral("framework")
         << QStringLiteral("list");

    if(debug) qDebug()<<"click"<<Utils::QtcProcess::joinArgs(args);
    proc.setArguments(args);
    proc.start();
    if (!proc.waitForFinished()) {
        proc.kill();
        return QStringList();
    }

    if(proc.exitCode() != 0 || proc.exitStatus() != QProcess::NormalExit)
        return QStringList();

    QStringList allFws = QString::fromLocal8Bit(proc.readAllStandardOutput()).split(QStringLiteral("\n"),QString::SkipEmptyParts);

    //reverse the list
    QStringList result;
    result.reserve( allFws.size() );
    std::reverse_copy( allFws.begin(), allFws.end(), std::back_inserter( result ) );

    return result;
#endif

#if 0
    if (!target) {
        QStringList items;
        QDir frameworksDir(QLatin1String(Constants::UBUNTU_CLICK_FRAMEWORKS_BASEPATH));

        if(!frameworksDir.exists())
            return items;

        QStringList availableFrameworkFiles = frameworksDir.entryList(QStringList()<<QLatin1String("*.framework"),
                                                                      QDir::Files | QDir::NoDotAndDotDot,
                                                                      QDir::Name | QDir::Reversed);

        QStringList availableFrameworks;
        foreach(QString fw, availableFrameworkFiles) {
            fw.replace(QLatin1String(".framework"),QString());
            availableFrameworks.append(fw);
        }

        if(debug) qDebug()<<"Available Frameworks on the host"<<availableFrameworks;
        return availableFrameworks;
    } else {
        //hardcode for now, click chroots are broken, click is not installed and even if its installed
        //it does not show any valid informations
        if(target->majorVersion == 14 && target->minorVersion == 10) {
            return QStringList() << QStringLiteral("ubuntu-sdk-14.10-qml-dev3")
                                 << QStringLiteral("ubuntu-sdk-14.10-qml-dev2")
                                 << QStringLiteral("ubuntu-sdk-14.10-qml-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.10-papi-dev2")
                                 << QStringLiteral("ubuntu-sdk-14.10-papi-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.10-html-dev2")
                                 << QStringLiteral("ubuntu-sdk-14.10-html-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.10-dev2")
                                 << QStringLiteral("ubuntu-sdk-14.10-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.04")
                                 << QStringLiteral("ubuntu-sdk-14.04-qml")
                                 << QStringLiteral("ubuntu-sdk-14.04-qml-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.04-papi")
                                 << QStringLiteral("ubuntu-sdk-14.04-papi-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.04-html")
                                 << QStringLiteral("ubuntu-sdk-14.04-html-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.04-dev1")
                                 << QStringLiteral("ubuntu-sdk-13.10");
        } else if (target->majorVersion == 14 && target->minorVersion == 4){
            return QStringList() << QStringLiteral("ubuntu-sdk-14.04")
                                 << QStringLiteral("ubuntu-sdk-14.04-qml")
                                 << QStringLiteral("ubuntu-sdk-14.04-qml-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.04-papi")
                                 << QStringLiteral("ubuntu-sdk-14.04-papi-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.04-html")
                                 << QStringLiteral("ubuntu-sdk-14.04-html-dev1")
                                 << QStringLiteral("ubuntu-sdk-14.04-dev1")
                                 << QStringLiteral("ubuntu-sdk-13.10");
        } else {
            return QStringList() << QStringLiteral("ubuntu-sdk-13.10");
        }
    }
#endif

    //quick and dirty fix for trusty dev env
    Q_UNUSED(target);
    static QStringList frameworks {
        QStringLiteral("ubuntu-sdk-14.10-qml-dev3"),
        QStringLiteral("ubuntu-sdk-14.10-qml-dev2"),
        QStringLiteral("ubuntu-sdk-14.10-qml-dev1"),
        QStringLiteral("ubuntu-sdk-14.10-papi-dev2"),
        QStringLiteral("ubuntu-sdk-14.10-papi-dev1"),
        QStringLiteral("ubuntu-sdk-14.10-html-dev2"),
        QStringLiteral("ubuntu-sdk-14.10-html-dev1"),
        QStringLiteral("ubuntu-sdk-14.10-dev2"),
        QStringLiteral("ubuntu-sdk-14.10-dev1"),
        QStringLiteral("ubuntu-sdk-14.04"),
        QStringLiteral("ubuntu-sdk-14.04-qml"),
        QStringLiteral("ubuntu-sdk-14.04-qml-dev1"),
        QStringLiteral("ubuntu-sdk-14.04-papi"),
        QStringLiteral("ubuntu-sdk-14.04-papi-dev1"),
        QStringLiteral("ubuntu-sdk-14.04-html"),
        QStringLiteral("ubuntu-sdk-14.04-html-dev1"),
        QStringLiteral("ubuntu-sdk-14.04-dev1"),
        QStringLiteral("ubuntu-sdk-13.10")
    };
    return frameworks;
}

/*!
 * \brief UbuntuClickTool::targetExists
 * checks if the target is still available
 */
bool UbuntuClickTool::targetExists(const UbuntuClickTool::Target &target)
{
    QPair<int,int> targetVer = targetVersion(target);
    if(targetVer.first == -1)
        return false;

    return true;
}

/*!
 * \brief UbuntuClickTool::getMostRecentFramework
 * returns the framework with the highest number supporting the subFramework
 * or a empty string of no framework with the given \a subFramework was found
 */
QString UbuntuClickTool::getMostRecentFramework(const QString &subFramework, const Target *target)
{
    //returned list is ordered from newest -> oldest framework
    QStringList allFws = getSupportedFrameworks(target);
    QString currFw;
    foreach(const QString &framework, allFws) {
        QString basename;
        QStringList extensions;
        QRegularExpression expr(QLatin1String(Constants::UBUNTU_CLICK_BASE_FRAMEWORK_REGEX));
        QRegularExpressionMatch match = expr.match(framework);
        if(match.hasMatch()) {
            basename = match.captured(1);
            extensions = QString(framework).replace(basename,
                                                    QString()).split(QChar::fromLatin1('-'),
                                                                     QString::SkipEmptyParts);
        } else {
            continue;
        }
        //this is a multi purpose framework
        if (extensions.size() == 0
                || (extensions.size() == 1 && extensions[0].startsWith(QLatin1String("dev")) )) {
            if (currFw.isEmpty()) {
                currFw = framework;
            }
            //if the subframework is empty we return
            //the first baseframework we can find
            if(subFramework.isEmpty())
                return currFw;
            continue;
        }

        if(extensions.contains(subFramework))
            return framework;
    }
    return currFw;
}

/**
 * @brief UbuntuClickTool::listAvailableTargets
 * @return all currently existing chroot targets in the system
 */
QList<UbuntuClickTool::Target> UbuntuClickTool::listAvailableTargets(const QString &framework)
{
    QList<Target> items;
    QDir chrootDir(QLatin1String(Constants::UBUNTU_CLICK_CHROOT_BASEPATH));
    QString filterRegex;
    filterRegex = QString::fromLatin1(Constants::UBUNTU_CLICK_TARGETS_REGEX).arg(clickChrootSuffix());
    if(!framework.isEmpty()) {
        QRegularExpression expr(QLatin1String(Constants::UBUNTU_CLICK_BASE_FRAMEWORK_REGEX));
        QRegularExpressionMatch match = expr.match(framework);
        if(match.hasMatch()) {
            if(debug) qDebug()<<"Filtering for base framework: "<<match.captured(1);
            filterRegex = QString::fromLatin1(Constants::UBUNTU_CLICK_TARGETS_FRAMEWORK_REGEX)
                        		     .arg(clickChrootSuffix())
                                             .arg(match.captured(1));
        }
    }

    //if the dir does not exist there are no available chroots
    if(!chrootDir.exists())
        return items;

    QStringList availableChroots = chrootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                       QDir::Name | QDir::Reversed);

    QRegularExpression clickFilter(filterRegex);
    //iterate over all chroots and check if they are click chroots
    foreach (const QString &chroot, availableChroots) {
        QRegularExpressionMatch match = clickFilter.match(chroot);
        if(!match.hasMatch()) {
            continue;
        }

        Target t;
        if(!targetFromPath(chroot,&t))
            continue;

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
    QFile f(QString::fromLatin1("%1/%2")
            .arg(targetBasePath(target))
            .arg(QLatin1String("etc/lsb-release")));

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

/*!
 * \brief UbuntuClickTool::targetFromPath
 * returns true if the given path is a click target
 * if it is, \a tg will be initialized with that targets values
 */
bool UbuntuClickTool::targetFromPath(const QString &targetPath, UbuntuClickTool::Target *tg)
{
    QRegularExpression clickFilter(QString::fromLatin1(Constants::UBUNTU_CLICK_TARGETS_REGEX).arg(clickChrootSuffix()));
    QRegularExpressionMatch match = clickFilter.match(targetPath);
    if(!match.hasMatch()) {
        return false;
    }

    Target t;
    t.maybeBroken  = false; //we are optimistic
    t.framework    = match.captured(1);
    t.architecture = match.captured(2);

    //now read informations about the target
    QFile f(QString::fromLatin1("%1/%2")
            .arg(targetBasePath(t))
            .arg(QLatin1String("/etc/lsb-release")));

    if (!f.open(QIODevice::ReadOnly)) {
        //there is no lsb-release file... what now?
        t.maybeBroken = true;

    } else {
        QString info = QString::fromLatin1(f.readAll());

        //read version
        QRegularExpression grep(QLatin1String(Constants::UBUNTU_CLICK_VERSION_REGEX),QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = grep.match(info);

        if(!match.hasMatch()) {
            t.maybeBroken = true;
        } else {
            bool ok = false;

            t.majorVersion = match.captured(1).toInt(&ok);
            if(!ok) {
                t.maybeBroken = true;
                t.majorVersion = -1;
            }

            t.minorVersion = match.captured(2).toInt(&ok);
            if(!ok) {
                t.maybeBroken = true;
                t.minorVersion = -1;
            }
        }

        //read series
        grep.setPattern(QString::fromLatin1(Constants::UBUNTU_CLICK_SERIES_REGEX));
        grep.setPatternOptions(QRegularExpression::MultilineOption);
        match = grep.match(info);

        if(!match.hasMatch()) {
            t.maybeBroken = true;
        } else {
            t.series = match.captured(1);
        }
    }

    *tg = t;
    return true;
}

/*!
 * \brief UbuntuClickTool::clickTargetFromTarget
 * Tries to get the Click target from a projectconfiguration,
 * \returns 0 if nothing was found
 */
const UbuntuClickTool::Target *UbuntuClickTool::clickTargetFromTarget(ProjectExplorer::Target *t)
{
#ifndef IN_TEST_PROJECT
    if(!t)
        return 0;

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(t->kit());
    if(!tc || (tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)))
        return 0;

    ClickToolChain *clickTc = static_cast<ClickToolChain*>(tc);
    if(!clickTc)
        return 0;

    return  &clickTc->clickTarget();
#else
    Q_UNUSED(t);
    return 0;
#endif
}

QString UbuntuClickTool::findOrCreateGccWrapper (const UbuntuClickTool::Target &target)
{
    QString compiler;

    if(target.architecture == QStringLiteral("armhf"))
        compiler = QStringLiteral("arm-linux-gnueabihf-gcc");
    else if(target.architecture == QStringLiteral("i386"))
        compiler = QStringLiteral("i686-linux-gnu-gcc");
    else if(target.architecture == QStringLiteral("amd64"))
        compiler = QStringLiteral("x86_64-linux-gnu-gcc");
    else {
        qWarning()<<"Invalid architecture, can not create gcc wrapper link";
        return QString();
    }

    return UbuntuClickTool::findOrCreateToolWrapper(compiler,target);
}

QString UbuntuClickTool::findOrCreateQMakeWrapper (const UbuntuClickTool::Target &target)
{
    QString qmake;

    if(target.architecture == QStringLiteral("armhf"))
        qmake = QStringLiteral("qmake-ubuntu-sdk-14.10-armhf");
    else
        qmake = QStringLiteral("qmake");

    return UbuntuClickTool::findOrCreateToolWrapper(qmake,target);
}

QString UbuntuClickTool::findOrCreateMakeWrapper (const UbuntuClickTool::Target &target)
{
    return UbuntuClickTool::findOrCreateToolWrapper(QStringLiteral("make"),target);
}

QString UbuntuClickTool::findOrCreateToolWrapper (const QString &tool, const UbuntuClickTool::Target &target)
{
    QString baseDir = QStringLiteral("%1/ubuntu-sdk/%2-%3").arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
            .arg(target.framework)
            .arg(target.architecture);

    QDir d(baseDir);
    if(!d.exists()) {
        if(!d.mkpath(baseDir)){
            qWarning()<<"Could not create config directory.";
            return QString();
        }
    }

    QString toolWrapper = (Utils::FileName::fromString(baseDir).appendPath(tool).toString());
    if(!QFile::exists(toolWrapper)) {
        if(!QFile::link(QString::fromLatin1(Constants::UBUNTU_CLICK_CHROOT_WRAPPER)
                        .arg(Constants::UBUNTU_SCRIPTPATH),toolWrapper)) {
            qWarning()<<"Unable to create link for the tool wrapper: "<<toolWrapper;
            return QString();
        }

    }
    return toolWrapper;
}

QDebug operator<<(QDebug dbg, const UbuntuClickTool::Target& t)
{
    dbg.nospace() << "("<<"series: "<<t.series<<" "
                        <<"arch: "<<t.architecture<<" "
                        <<"framework: "<<t.framework<<" "
                        <<"version: "<<t.majorVersion<<"."<<t.minorVersion<<" "
                        <<"broken "<<t.maybeBroken
                        <<")";

    return dbg.space();
}

} // namespace Internal
} // namespace Ubuntu

