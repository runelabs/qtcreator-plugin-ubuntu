#include "ubuntukitmanager.h"
#include "clicktoolchain.h"
#include "ubuntuconstants.h"
#include "ubuntucmaketool.h"
#include "ubuntudevice.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitinformation.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>

namespace Ubuntu {
namespace Internal {

static bool equalKits(ProjectExplorer::Kit *a, ProjectExplorer::Kit *b)
{
    return ProjectExplorer::ToolChainKitInformation::toolChain(a) == ProjectExplorer::ToolChainKitInformation::ToolChainKitInformation::toolChain(b);
}

UbuntuKitManager::UbuntuKitManager()
{
}

void UbuntuKitManager::autoDetectKits()
{
    QList<ClickToolChain *> toolchains;
    // having a empty toolchains list will remove all autodetected kits for android
    // exactly what we want in that case
    foreach (ProjectExplorer::ToolChain *tc, ProjectExplorer::ToolChainManager::toolChains()) {
        if(tc) {
            if (!tc->isAutoDetected())
                continue;
            if (tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
                continue;
            toolchains << static_cast<ClickToolChain *>(tc);
        }
    }

    QList<ProjectExplorer::Kit *> existingKits;
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits()) {
        if (!k->isAutoDetected())
            continue;
        if (k->isSdkProvided())
            continue;

        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
        if (tc && tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
            continue;

        CMakeProjectManager::ICMakeTool* icmake = CMakeProjectManager::CMakeKitInformation::cmakeTool(k);
        UbuntuCMakeTool* cmake = qobject_cast<UbuntuCMakeTool*>(icmake);
        //if the kit has no valid UbuntuCMakeTool, let it be removed from the code later
        if(cmake) {
            Utils::Environment env = Utils::Environment::systemEnvironment();
            k->addToEnvironment(env);
            cmake->setEnvironment(env);
        }

        //@TODO check for ubuntu device information
        qDebug()<<"Found possible Ubuntu Kit: "<<k->displayName();
        existingKits << k;
    }

#if 0
    QMap<Abi::Architecture, QList<QtSupport::BaseQtVersion *> > qtVersionsForArch;
    foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::versions()) {
        if (qtVersion->type() != QLatin1String(Constants::ANDROIDQT))
            continue;
        QList<Abi> qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        qtVersionsForArch[qtAbis.first().architecture()].append(qtVersion);
    }

    DeviceManager *dm = DeviceManager::instance();
    IDevice::ConstPtr device = dm->find(Core::Id(Constants::ANDROID_DEVICE_ID));
    if (device.isNull()) {
        // no device, means no sdk path
        foreach (Kit *k, existingKits)
            KitManager::deregisterKit(k);
        return;
    }
#endif

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
                oldKit->makeSticky();

                //make sure kit has all required informations
                fixKit(oldKit);

                newKits.removeAt(j);
                ProjectExplorer::KitManager::deleteKit(newKit);
                j = newKits.count();
            }
        }
    }

    //all kits remaining need to be removed if they don't have all informations
    foreach (ProjectExplorer::Kit *k, existingKits) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
        CMakeProjectManager::ICMakeTool* icmake = CMakeProjectManager::CMakeKitInformation::cmakeTool(k);
        if (tc && tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
                && icmake && icmake->id().toString().startsWith(QLatin1String(Constants::UBUNTU_CLICK_CMAKE_TOOL_ID))
                && icmake->isValid()) {
            //existing targets are not autodetected anymore
            k->makeUnSticky();
            k->setAutoDetected(false);
            fixKit(k);
        } else {
            //has not all informations, go away
            ProjectExplorer::KitManager::deregisterKit(k);
        }
    }

    foreach (ProjectExplorer::Kit *kit, newKits) {
        ClickToolChain *tc = static_cast<ClickToolChain *>(ProjectExplorer::ToolChainKitInformation::toolChain(kit));
        //AndroidQtVersion *qt = static_cast<AndroidQtVersion *>(QtSupport::QtKitInformation::qtVersion(kit));
        kit->setDisplayName(tr("UbuntuSDK for %1 (GCC %2-%3)")
                            .arg(tc->clickTarget().architecture)
                            .arg(tc->clickTarget().framework)
                            .arg(tc->clickTarget().series));
        ProjectExplorer::KitManager::registerKit(kit);
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
    newKit->setAutoDetected(true);
    newKit->setIconPath(Utils::FileName::fromString(QLatin1String(Constants::UBUNTU_MODE_WEB_ICON)));
    ProjectExplorer::ToolChainKitInformation::setToolChain(newKit, tc);

    //every kit gets its own instance of CMakeTool
    UbuntuCMakeTool* cmake = new UbuntuCMakeTool;
    Utils::Environment env = Utils::Environment::systemEnvironment();
    newKit->addToEnvironment(env);
    cmake->setEnvironment(env);

    CMakeProjectManager::CMakeToolManager::registerCMakeTool(cmake);
    CMakeProjectManager::CMakeKitInformation::setCMakeTool(newKit,cmake->id());

    //@TODO add gdbserver support
    //@TODO add real qt version
    QtSupport::QtKitInformation::setQtVersion(newKit, 0);

    fixKit(newKit);
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
    debugger.setDisplayName(tr("Ubuntu SDK Debugger"));
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
    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if(!tc) {
        //make kit editable for user (i think)
        k->setAutoDetected(false);
        return;
    }

    //make sure we have the multiarch debugger
    if(!Debugger::DebuggerKitInformation::debugger(k)) {
        QVariant dId = createOrFindDebugger(tc->suggestedDebugger());
        if(dId.isValid())
            Debugger::DebuggerKitInformation::setDebugger(k,dId);
    }

    //make sure we point to a ubuntu device
    Core::Id devId = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k);
    if(devId != Constants::UBUNTU_DEVICE_ID || !devId.isValid()) {
        ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k,Core::Id(Constants::UBUNTU_DEVICE_ID));
        ProjectExplorer::DeviceKitInformation::setDevice(k,ProjectExplorer::IDevice::ConstPtr());
    }

    //values the user can change
    k->setSticky(ProjectExplorer::DeviceKitInformation::id(),false);
    k->setSticky(Debugger::DebuggerKitInformation::id(),false);

}

} // namespace Internal
} // namespace Ubuntu
