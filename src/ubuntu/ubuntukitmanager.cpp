#include "ubuntukitmanager.h"
#include "clicktoolchain.h"
#include "ubuntuconstants.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitinformation.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/qtkitinformation.h>

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
        if (!tc->isAutoDetected())
            continue;
        if (tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
            continue;
        toolchains << static_cast<ClickToolChain *>(tc);
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
                newKits.removeAt(j);
                existingKits.at(i)->makeSticky();
                existingKits.removeAt(i);
                ProjectExplorer::KitManager::deleteKit(newKit);
                j = newKits.count();
            }
        }
    }

    //all kits remaining need to be removed if they don't have all informations
    foreach (ProjectExplorer::Kit *k, existingKits) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
        if(!tc) {
            qDebug()<<"Found Kit with empty TC";
        } else {
            if(tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
                qDebug()<<"Found Kit with wrong type "<<tc->type();
        }
        if (tc && tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
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
        //AndroidQtVersion *qt = static_cast<AndroidQtVersion *>(QtSupport::QtKitInformation::qtVersion(kit));
        kit->setDisplayName(tr("UbuntuSDK for %1 (GCC %2-%3)")
                            .arg(tc->clickTarget().architecture)
                            .arg(tc->clickTarget().framework)
                            .arg(tc->clickTarget().series));
        ProjectExplorer::KitManager::registerKit(kit);
    }
}

ProjectExplorer::Kit *UbuntuKitManager::createKit(ClickToolChain *tc)
{
    //@TODO find a qt version
    ProjectExplorer::Kit* newKit = new ProjectExplorer::Kit;
    newKit->setAutoDetected(true);
    newKit->setIconPath(Utils::FileName::fromString(QLatin1String(Constants::UBUNTU_MODE_WEB_ICON)));
    ProjectExplorer::ToolChainKitInformation::setToolChain(newKit, tc);

    Debugger::DebuggerItem debugger;
    debugger.setCommand(tc->suggestedDebugger());
    debugger.setEngineType(Debugger::GdbEngineType);
    debugger.setDisplayName(tr("Ubuntu Debugger for %1").arg(tc->displayName()));
    debugger.setAutoDetected(true);
    debugger.setAbi(tc->targetAbi());
    QVariant id = Debugger::DebuggerItemManager::registerDebugger(debugger);
    Debugger::DebuggerKitInformation::setDebugger(newKit, id);

    //@TODO add gdbserver support
    //@TODO add device type
    //@TODO add real qt version
    QtSupport::QtKitInformation::setQtVersion(newKit, 0);

    return newKit;
}

} // namespace Internal
} // namespace Ubuntu
