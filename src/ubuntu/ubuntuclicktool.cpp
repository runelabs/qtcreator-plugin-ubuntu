#include "ubuntuclicktool.h"

#include <QRegExp>
#include <QDir>
#include <QDebug>

#include <utils/qtcprocess.h>

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

void UbuntuClickTool::parametersForCreateChroot(const QString &arch, const QString &series, ProjectExplorer::ProcessParameters *params)
{
    params->setCommand(QLatin1String("click"));

    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << arch
              << QLatin1String("-s")
              << series
              << QLatin1String("create");

    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
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
        strMode = QLatin1String("delete");
        break;
    }

    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << strMode;

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
}

/**
 * @brief UbuntuClickTool::parametersForMake
 * Fills ProcessParameters to run make inside the Target
 * @note does not call ProjectExplorer::ProcessParameters::resolveAll()
 */
void UbuntuClickTool::parametersForMake(const UbuntuClickTool::Target &target, const QString &buildDir
                                        , ProjectExplorer::ProcessParameters *params)
{
    QStringList arguments;
    arguments << QLatin1String("chroot")
              << QLatin1String("-a")
              << target.architecture
              << QLatin1String("-f")
              << target.framework
              << QLatin1String("run")
              << QLatin1String("make");

    params->setWorkingDirectory(buildDir);
    params->setCommand(QLatin1String("click"));
    params->setArguments(Utils::QtcProcess::joinArgs(arguments));
}

/**
 * @brief UbuntuClickTool::listAvailableTargets
 * @return all currently existing chroot targets in the system
 */
QList<UbuntuClickTool::Target> UbuntuClickTool::listAvailableTargets()
{
    QList<Target> items;
    QDir chrootDir(QLatin1String("/var/lib/schroot/chroots"));

    //if the dir does not exist there are no available chroots
    if(!chrootDir.exists())
        return items;

    QStringList availableChroots = chrootDir.entryList(QDir::Dirs);

    QRegExp clickFilter(QLatin1String("^click-(.*)-([A-Za-z0-9]+)$"));

    //iterate over all chroots and check if they are click chroots
    foreach (const QString &chroot, availableChroots) {
        if(!clickFilter.exactMatch(chroot)) {
            qDebug()<<"Skipping: "<<chroot<<" Matched: "<<clickFilter.matchedLength();
            continue;
        }

        //we need at least 3 captures
        if ( clickFilter.captureCount() < 2 ) {
            qDebug()<<"Skipping: "<<chroot<<" Not enough matches: "<<clickFilter.capturedTexts();
            continue;
        }

        Target t;
        t.framework    = clickFilter.cap(1);
        t.architecture = clickFilter.cap(2);
        items.append(t);
    }

    return items;
}

} // namespace Internal
} // namespace Ubuntu
