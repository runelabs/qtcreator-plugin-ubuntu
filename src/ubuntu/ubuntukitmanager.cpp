#include "ubuntukitmanager.h"
#include "clicktoolchain.h"
#include "ubuntuconstants.h"
#include <ubuntu/device/remote/ubuntudevice.h>
#include "ubuntuclickdialog.h"
#include "ubuntuqtversion.h"
#include "settings.h"
#include "device/container/containerdevice.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/qtkitinformation.h>

#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <cmakeprojectmanager/cmaketool.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include <QPair>
#include <QInputDialog>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

static bool equalKits(ProjectExplorer::Kit *a, ProjectExplorer::Kit *b)
{
    return ProjectExplorer::ToolChainKitInformation::toolChain(a) == ProjectExplorer::ToolChainKitInformation::ToolChainKitInformation::toolChain(b);
}

static bool lessThanToolchain (const ClickToolChain* left, const ClickToolChain* right)
{
    const UbuntuClickTool::Target &leftTarget = left->clickTarget();
    const UbuntuClickTool::Target &rightTarget = right->clickTarget();
    return UbuntuClickFrameworkProvider::caseInsensitiveFWLessThan(leftTarget.framework, rightTarget.framework);
}


static void createOrFindDeviceAndType(ProjectExplorer::Kit *k, ClickToolChain *tc)
{
    if (UbuntuClickTool::compatibleWithHostArchitecture(tc->clickTarget().architecture)) {
        Core::Id devId = ContainerDevice::createIdForContainer(tc->clickTarget().containerName);
        ProjectExplorer::IDevice::ConstPtr ptr
                = ProjectExplorer::DeviceManager::instance()->find(devId);

        if (!ptr) {
            ContainerDevice::Ptr dev = ContainerDevice::create(devId, devId);
            ProjectExplorer::DeviceManager::instance()->addDevice(dev);
            ptr = dev;
        }

        ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k,devId);
        ProjectExplorer::DeviceKitInformation::setDevice(k, ptr);
    } else {
        //a Kit that cannot take a Container Device
        Core::Id devTypeId = Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix(tc->clickTarget().architecture);
        ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k,devTypeId);
    }
}
UbuntuKitManager::UbuntuKitManager()
{
}

QList<ClickToolChain *> UbuntuKitManager::clickToolChains()
{
    QList<ClickToolChain *> toolchains;
    // having a empty toolchains list will remove all autodetected kits for android
    // exactly what we want in that case
    foreach (ProjectExplorer::ToolChain *tc, ProjectExplorer::ToolChainManager::toolChains()) {
        if(tc) {
            if (!tc->isAutoDetected())
                continue;
            if (tc->typeId() != Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
                continue;
            toolchains << static_cast<ClickToolChain *>(tc);
        }
    }
    return toolchains;
}

QList<ProjectExplorer::Kit *> UbuntuKitManager::findKitsUsingTarget (const UbuntuClickTool::Target &target)
{
    auto matcher = [&target](const ProjectExplorer::Kit *k) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
        if (!tc)
            return false;

        if (tc->typeId() != Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
            return false;

        ClickToolChain *cTc = static_cast<ClickToolChain *>(tc);
        return (cTc->clickTarget().containerName == target.containerName);
    };

    return ProjectExplorer::KitManager::matchingKits(ProjectExplorer::KitMatcher(matcher));
}

UbuntuQtVersion *UbuntuKitManager::createOrFindQtVersion(ClickToolChain *tc)
{
    QString qmakePath = UbuntuClickTool::findOrCreateQMakeWrapper(tc->clickTarget());
    if(!QFile::exists(qmakePath)) {
        return 0;
    } else {
        //try to find a already existing Qt Version for this chroot
        foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::versions()) {
            if (qtVersion->type() != QLatin1String(Constants::UBUNTU_QTVERSION_TYPE))
                continue;

            if (qtVersion->qmakeCommand().toFileInfo().absoluteFilePath() == QFileInfo(qmakePath).absoluteFilePath())
                return static_cast<UbuntuQtVersion*> (qtVersion);
        }
    }

    UbuntuQtVersion *qtVersion = new UbuntuQtVersion(tc->clickTarget().containerName, Utils::FileName::fromString(qmakePath),false);
    QtSupport::QtVersionManager::addVersion(qtVersion);
    return qtVersion;
}

CMakeProjectManager::CMakeTool *UbuntuKitManager::createOrFindCMakeTool(ClickToolChain *tc)
{
    QString cmakePathStr = UbuntuClickTool::findOrCreateToolWrapper(QStringLiteral("cmake"), tc->clickTarget());
    Utils::FileName cmakePath = Utils::FileName::fromString(cmakePathStr);

    CMakeProjectManager::CMakeTool *cmake = CMakeProjectManager::CMakeToolManager::findByCommand(cmakePath);
    if (cmake)
        return cmake;

    cmake = createCMakeTool(tc);
    if (!CMakeProjectManager::CMakeToolManager::registerCMakeTool(cmake)) {
        delete cmake;
        return 0;
    }

    return cmake;
}

CMakeProjectManager::CMakeTool *UbuntuKitManager::createCMakeTool(ClickToolChain *tc)
{
    return createCMakeTool(tc->clickTarget());
}

CMakeProjectManager::CMakeTool *UbuntuKitManager::createCMakeTool(const UbuntuClickTool::Target &target)
{
    QString cmakePathStr = UbuntuClickTool::findOrCreateToolWrapper(QStringLiteral("cmake"), target);
    Utils::FileName cmakePath = Utils::FileName::fromString(cmakePathStr);
    CMakeProjectManager::CMakeTool *cmake = new CMakeProjectManager::CMakeTool(CMakeProjectManager::CMakeTool::AutoDetection,
                                                                               CMakeProjectManager::CMakeTool::createId());

    cmake->setCMakeExecutable(cmakePath);
    cmake->setDisplayName(tr("Ubuntu SDK cmake (%1-%2-%3)")
                          .arg(target.architecture)
                          .arg(target.framework)
                          .arg(target.containerName));
    return cmake;
}

void UbuntuKitManager::autoCreateKit(UbuntuDevice::Ptr device)
{
    ProjectExplorer::Abi requiredAbi = ClickToolChain::architectureNameToAbi(device->architecture());
    if(requiredAbi.isNull()) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Unknown device architecture"),
                             tr("Kit autocreation for %1 is not supported!")
                             .arg(device->architecture()));
        return;
    }

    if(device->framework().isEmpty()) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Device framework is unknown."),
                             tr("The supported framework of the device is not known, please make sure to redetect the device features."));
        return;
    }

    QList<ClickToolChain*> toolchains = clickToolChains();

    auto findCompatibleTc = [&](){
        QList<ClickToolChain *> perfectMatches;
        QList<ClickToolChain *> fuzzyMatches;
        if(toolchains.size() > 0) {
            qSort(toolchains.begin(),toolchains.end(),lessThanToolchain);

            for( int i = toolchains.size() -1; i >= 0; i-- ) {
                ClickToolChain* tc = toolchains[i];

                if (tc->clickTarget().framework != device->framework())
                    continue;

                if( tc->targetAbi() == requiredAbi ) {
                    perfectMatches.append(tc);
                    continue;
                }

                //the abi is compatible but not exactly the same
                if(tc->targetAbi().isCompatibleWith(requiredAbi))
                    fuzzyMatches.append(tc);
            }
        }
        return qMakePair(perfectMatches, fuzzyMatches);
    };

    ClickToolChain* match = nullptr;
    while (!match) {

        //search a tk with a compatible arch
        QPair<QList<ClickToolChain *>, QList<ClickToolChain *> > matches = findCompatibleTc();
        QList<ClickToolChain *> perfect = matches.first;
        QList<ClickToolChain *> fuzzy = matches.second;

        auto getToolchain = [](const QList<ClickToolChain *> &toolchains, bool *ok) {
            QStringList names;
            foreach (const ClickToolChain *curr, toolchains) {
                names.append(curr->displayName());
            }

            if (names.empty())
                return static_cast<ClickToolChain *>(nullptr);

            QString selection = QInputDialog::getItem(Core::ICore::mainWindow(), qApp->applicationName(),
                                                      tr("There are multiple compatible Toolchains available, please select one:"),
                                                      names, 0, false, ok);
            if (!ok) {
                return static_cast<ClickToolChain *>(nullptr);
            }

            return toolchains.at(names.indexOf(selection));
        };

        if (perfect.size() > 0) {
            if (perfect.size() == 1) {
                match = perfect.first();
                break;
            } else {
                bool ok = true;
                match = getToolchain(perfect, &ok);
                if (!ok)
                    return;

                break;
            }
        } else if (fuzzy.size() > 0) {
            if (fuzzy.size() == 1) {
                match = fuzzy.first();
                break;
            } else {
                bool ok = true;
                match = getToolchain(fuzzy, &ok);
                if (!ok)
                    return;

                break;
            }
        }

        if (!match) {
            //create target
            int choice = QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("No target available"),
                                  tr("There is no compatible target available on your system, do you want to create it now?"));

            if(choice == QMessageBox::Yes) {
                if(!UbuntuClickDialog::createClickChrootModal(false, device->architecture(), device->framework()))
                    return;
                toolchains = clickToolChains();
            } else
                return;
        }
    }

    ProjectExplorer::Kit* newKit = createKit(match);
    if(newKit) {
        fixKit(newKit);

        newKit->setUnexpandedDisplayName(tr("%1 (GCC %2-%3-%4)")
                                        .arg(device->displayName())
                                        .arg(match->clickTarget().architecture)
                                        .arg(match->clickTarget().framework)
                                        .arg(match->clickTarget().containerName));

        ProjectExplorer::DeviceKitInformation::setDevice(newKit,device);
        ProjectExplorer::KitManager::registerKit(newKit);
    }
}

void UbuntuKitManager::autoDetectKits()
{
    //destroy all obsolete QtVersions, they are recreated later on in fixKit()
    foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::versions()) {
        if (qtVersion->type() != QLatin1String(Constants::UBUNTU_QTVERSION_TYPE))
            continue;

        UbuntuQtVersion* ver = static_cast<UbuntuQtVersion*> (qtVersion);
        if(ver->scriptVersion() < UbuntuQtVersion::minimalScriptVersion() || !qtVersion->isValid()) {
            //we need to remove that QtVersion
            QFile::remove(ver->qmakeCommand().toString());
            QtSupport::QtVersionManager::removeVersion(ver);
        }
    }

    // having a empty toolchains list will remove all autodetected kits for ubuntu
    // exactly what we want in that case
    QList<ClickToolChain *> toolchains = clickToolChains();

    QList<ProjectExplorer::Kit *> existingKits;
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits()) {
        if (k->isSdkProvided())
            continue;

        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
        if (tc && tc->typeId() != Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
            continue;

        //@TODO check for ubuntu device information
        if(debug) qDebug()<<"Found possible Ubuntu Kit: "<<k->displayName();
        existingKits << k;
    }

    // create new kits
    QList<ProjectExplorer::Kit *> newKits;
    foreach (ClickToolChain *tc, toolchains) {
        ProjectExplorer::Kit* newKit = createKit(tc);
        newKit->makeSticky();
        newKits << newKit;
    }

    //remove already existing kits
    for (int i = existingKits.count() - 1; i >= 0; --i) {
        ProjectExplorer::Kit *existingKit = existingKits.at(i);
        for (int j = 0; j < newKits.count(); ++j) {
            ProjectExplorer::Kit *newKit = newKits.at(j);
            if (equalKits(existingKit, newKit)) {
                // Kit is already registered, nothing to do
                ProjectExplorer::Kit *oldKit = existingKits.takeAt(i);
                oldKit->blockNotification();
                oldKit->makeSticky();

                //make sure kit has all required informations
                fixKit(oldKit);
                oldKit->unblockNotification();

                newKits.removeAt(j);
                ProjectExplorer::KitManager::deleteKit(newKit);
                j = newKits.count();
            }
        }
    }

    //all kits remaining need to be removed if they don't have all informations
    foreach (ProjectExplorer::Kit *k, existingKits) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
        CMakeProjectManager::CMakeTool* cmake = CMakeProjectManager::CMakeKitInformation::cmakeTool(k);
        if (tc && tc->typeId() == Constants::UBUNTU_CLICK_TOOLCHAIN_ID
                && cmake
                && cmake->isValid()) {
            fixKit(k);

            //existing targets are not autodetected anymore
            k->makeUnSticky();
            k->setAutoDetected(false);
        } else {
            //has not all informations, go away
            ProjectExplorer::KitManager::deregisterKit(k);
        }
    }

    foreach (ProjectExplorer::Kit *kit, newKits) {
        ClickToolChain *tc = static_cast<ClickToolChain *>(ProjectExplorer::ToolChainKitInformation::toolChain(kit));
        kit->setUnexpandedDisplayName(tr("UbuntuSDK for %1 (GCC %2-%3)")
                                      .arg(tc->clickTarget().containerName)
                                      .arg(tc->clickTarget().architecture)
                                      .arg(tc->clickTarget().framework));
        ProjectExplorer::KitManager::registerKit(kit);
        fixKit(kit);
    }

    static bool cmakeUpdaterSet = false;
    if (!cmakeUpdaterSet) {

        cmakeUpdaterSet = true;

        auto cmakeUpdater = [](const Core::Id &id){
            CMakeProjectManager::CMakeTool *tool = CMakeProjectManager::CMakeToolManager::findById(id);
            if (!tool)
                return;

            QString basePath = Settings::settingsPath().toString();
            if (tool->cmakeExecutable().toString().startsWith(basePath)) {
                qDebug()<<"Setting mapper to "<<tool->displayName();
                tool->setPathMapper(&UbuntuClickTool::mapIncludePathsForCMake);
            } else {
                qDebug()<<"Unsetting mapper from "<<tool->displayName();
                tool->setPathMapper(CMakeProjectManager::CMakeTool::PathMapper());
            }
        };

        connect(CMakeProjectManager::CMakeToolManager::instance(), &CMakeProjectManager::CMakeToolManager::cmakeAdded,
                cmakeUpdater);
        connect(CMakeProjectManager::CMakeToolManager::instance(), &CMakeProjectManager::CMakeToolManager::cmakeRemoved,
                cmakeUpdater);
    }
}

/*!
 * \brief UbuntuKitManager::createKit
 * Creates a new Kit for the Ubunut toolchain and sets default
 * values
 */
ProjectExplorer::Kit *UbuntuKitManager::createKit(ClickToolChain *tc)
{
    //@TODO find a qt version
    ProjectExplorer::Kit* newKit = new ProjectExplorer::Kit;
    newKit->setAutoDetected(false); //let the user delete that stuff
    newKit->setIconPath(Utils::FileName::fromString(QLatin1String(Constants::UBUNTU_ICON)));
    ProjectExplorer::ToolChainKitInformation::setToolChain(newKit, tc);

    CMakeProjectManager::CMakeTool *cmake = createOrFindCMakeTool(tc);
    if (cmake) {
        cmake->setPathMapper(&UbuntuClickTool::mapIncludePathsForCMake);
        CMakeProjectManager::CMakeKitInformation::setCMakeTool(newKit, cmake->id());
    }

    ProjectExplorer::SysRootKitInformation::setSysRoot(newKit,Utils::FileName::fromString(UbuntuClickTool::targetBasePath(tc->clickTarget())));

    createOrFindDeviceAndType(newKit, tc);

    //@TODO add gdbserver support
    QtSupport::QtKitInformation::setQtVersion(newKit, createOrFindQtVersion(tc));
    return newKit;
}



/*!
 * \brief UbuntuKitManager::createOrFindDebugger
 * Tries to find a already existing ubuntu debugger, if it can not find one
 * it is registered and returned
 */
QVariant UbuntuKitManager::createOrFindDebugger(const Utils::FileName &path)
{
    if(path.isEmpty())
        return QVariant();

    QList<Debugger::DebuggerItem> debuggers = Debugger::DebuggerItemManager::debuggers();
    foreach(const Debugger::DebuggerItem& debugger,debuggers) {
        if(debugger.command() == path) {
            return debugger.id();
        }
    }


    Debugger::DebuggerItem debugger;
    debugger.setCommand(path);
    debugger.setEngineType(Debugger::GdbEngineType);
    debugger.setUnexpandedDisplayName(tr("Ubuntu SDK Debugger"));
    debugger.setAutoDetected(true);
    //multiarch debugger
    ProjectExplorer::Abi abi(ProjectExplorer::Abi::UnknownArchitecture
                             ,ProjectExplorer::Abi::LinuxOS
                             ,ProjectExplorer::Abi::GenericLinuxFlavor
                             ,ProjectExplorer::Abi::UnknownFormat
                             ,0);
    debugger.setAbi(abi);
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

/*!
 * \brief UbuntuKitManager::fixKit
 * Tries to fix a Kit if there is missing information
 */
void UbuntuKitManager::fixKit(ProjectExplorer::Kit *k)
{
    k->setAutoDetected(false);

    ClickToolChain* tc = static_cast<ClickToolChain *> (ProjectExplorer::ToolChainKitInformation::toolChain(k));
    if(!tc) {
        return;
    }

    //make sure we have the multiarch debugger
    QVariant dId = createOrFindDebugger(tc->suggestedDebugger());
    const Debugger::DebuggerItem *debugger = Debugger::DebuggerKitInformation::debugger(k);
    if(!debugger) {
        if(dId.isValid())
            Debugger::DebuggerKitInformation::setDebugger(k,dId);
    } else if (debugger->id() != dId){
        if(dId.isValid())
            Debugger::DebuggerKitInformation::setDebugger(k,dId);
    }

    if(ProjectExplorer::SysRootKitInformation::sysRoot(k).isEmpty()) {
        ProjectExplorer::SysRootKitInformation::setSysRoot(k,Utils::FileName::fromString(UbuntuClickTool::targetBasePath(tc->clickTarget())));
    }

    //make sure we point to a ubuntu device
    Core::Id devId = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k);
    bool devValid        = devId.isValid(); //invalid type
    bool usesOldDevType  = devId == Constants::UBUNTU_DEVICE_TYPE_ID; //Kit uses still the old device type ids
    bool hasWrongDevType = devId.toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID)) ||
            devId.toString().startsWith(QLatin1String(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID)); //kit has a wrong device type

    if (usesOldDevType) {
        //if this kit still uses the old type ids, we try to find the correct device by name

        Core::Id devTypeId = Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix(tc->clickTarget().architecture);
        ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k,devTypeId);
        UbuntuDevice::ConstPtr fuzzyMatch;
        UbuntuDevice::ConstPtr fullMatch;

        //lets search for a device
        ProjectExplorer::DeviceManager *devMgr = ProjectExplorer::DeviceManager::instance();
        for (int i = 0; i<devMgr->deviceCount(); i++) {
            ProjectExplorer::IDevice::ConstPtr dev = devMgr->deviceAt(i);
            if(!dev)
                continue;

            //the type ID also checks if the architecture is correct
            if(dev->type() != devTypeId)
                continue;

            UbuntuDevice::ConstPtr ubuntuDev = qSharedPointerCast<const UbuntuDevice>(dev);

            //we found a possible Device!
            if(!fuzzyMatch)
                fuzzyMatch = ubuntuDev;

            //this is most likely the device that was used with this kit by using the autocreate button
            QRegularExpression regExp (QStringLiteral("^(%1\\s+\\(.*\\))$").arg(ubuntuDev->displayName()));
            QRegularExpressionMatch m = regExp.match(ubuntuDev->displayName());
            if (m.hasMatch()) {
                fullMatch = ubuntuDev;
                break;
            }
        }
        ProjectExplorer::DeviceKitInformation::setDevice(k,!fullMatch.isNull() ? fullMatch : fuzzyMatch);
    }

    if (!devValid || hasWrongDevType) {
        createOrFindDeviceAndType(k, tc);
    }

    //values the user can change
    k->setSticky(ProjectExplorer::DeviceKitInformation::id(),false);
    k->setSticky(Debugger::DebuggerKitInformation::id(),false);

    //values the user cannot change
    k->setSticky(ProjectExplorer::SysRootKitInformation::id(),true);
    k->setMutable(ProjectExplorer::SysRootKitInformation::id(),false);

    //make sure we use a ubuntu Qt version
    QtSupport::QtKitInformation::setQtVersion(k, createOrFindQtVersion(tc));

    //make sure we use a ubuntu cmake
    CMakeProjectManager::CMakeTool *cmake = createOrFindCMakeTool(tc);
    if(cmake) {
        cmake->setPathMapper(&UbuntuClickTool::mapIncludePathsForCMake);
        CMakeProjectManager::CMakeKitInformation::setCMakeTool(k, cmake->id());
    }

}

} // namespace Internal
} // namespace Ubuntu
