#include "ubuntudeploystepfactory.h"
#include "ubuntudirectuploadstep.h"

#include <ubuntu/ubuntuprojecthelper.h>
#include <ubuntu/ubuntuconstants.h>
#include <ubuntu/ubuntupackagestep.h>

#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qmlprojectmanager/qmlprojectconstants.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

namespace Ubuntu {
namespace Internal {

QList<ProjectExplorer::BuildStepInfo> UbuntuDeployStepFactory::availableSteps(ProjectExplorer::BuildStepList *parent) const
{
    QList<ProjectExplorer::BuildStepInfo> types;

    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return types;

    Core::Id targetDevice = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->target()->kit());
    if(targetDevice != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE && !targetDevice.toString().startsWith(QLatin1String(Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID)))
        return types;

    bool isRemote = targetDevice.toString().startsWith(QLatin1String(Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID));
    bool isCMake  = parent->target()->project()->id() == CMakeProjectManager::Constants::CMAKEPROJECT_ID;
    bool isHTML   = parent->target()->project()->id() == Ubuntu::Constants::UBUNTUPROJECT_ID;
    bool isQML    = parent->target()->project()->id() == "QmlProjectManager.QmlProject";
    bool isQMake  = parent->target()->project()->id() == QmakeProjectManager::Constants::QMAKEPROJECT_ID;

    if (isRemote) {
        //IF we have a remote device we just support a ubuntu toolchain
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(parent->target()->kit());
        if(tc && tc->typeId() != Ubuntu::Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
            return types;
    }

    if(isRemote && ( isHTML || isQML || isCMake || isQMake ) ) {

        types << ProjectExplorer::BuildStepInfo (Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID,
                                                 UbuntuDirectUploadStep::displayName(),
                                                 ProjectExplorer::BuildStepInfo::UniqueStep)
              << ProjectExplorer::BuildStepInfo (Constants::UBUNTU_CLICK_PACKAGESTEP_ID,
                                                 tr("UbuntuSDK create click package", "Display name for UbuntuPackageStep id."),
                                                 ProjectExplorer::BuildStepInfo::UniqueStep)
              //backwards compatibility to older projects
              << ProjectExplorer::BuildStepInfo (Constants::UBUNTU_DEPLOY_MAKESTEP_ID,
                                                 tr("UbuntuSDK create click package", "Display name for UbuntuPackageStep id."),
                                                 ProjectExplorer::BuildStepInfo::Flags(ProjectExplorer::BuildStepInfo::UniqueStep
                                                 | ProjectExplorer::BuildStepInfo::Uncreatable
                                                 | ProjectExplorer::BuildStepInfo::Unclonable));
    }

    return types;
}

bool UbuntuDeployStepFactory::canHandle(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    return Utils::contains(availableSteps(parent), Utils::equal(&ProjectExplorer::BuildStepInfo::id, id));
}

ProjectExplorer::BuildStep *UbuntuDeployStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canHandle(parent, id))
        return 0;

    if(id == Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID)
        return new UbuntuDirectUploadStep(parent);
    else if (id == Constants::UBUNTU_CLICK_PACKAGESTEP_ID) {
        UbuntuPackageStep *step = new UbuntuPackageStep(parent);
        return step;
    }

    return 0;
}

ProjectExplorer::BuildStep *UbuntuDeployStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    Core::Id id = ProjectExplorer::idFromMap(map);
    if(!canHandle(parent,id))
        return 0;

    if( id == Constants::UBUNTU_DEPLOY_MAKESTEP_ID ) {
        UbuntuPackageStep *step = new UbuntuPackageStep(parent);
        return step;
    }

    ProjectExplorer::BuildStep* step = create(parent,id);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }

    return step;
}

ProjectExplorer::BuildStep *UbuntuDeployStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canHandle(parent, product->id()))
        return 0;

    const Core::Id id = product->id();
    if(id == Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID)
        return new UbuntuDirectUploadStep(parent, static_cast<UbuntuDirectUploadStep *>(product));
    else if(id == Core::Id(Constants::UBUNTU_CLICK_PACKAGESTEP_ID))
        return new UbuntuPackageStep(parent, static_cast<UbuntuPackageStep *>(product));

    return 0;
}

} // namespace Internal
} // namespace Ubuntu
