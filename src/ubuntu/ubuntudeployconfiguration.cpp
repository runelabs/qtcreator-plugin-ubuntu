#include "ubuntudeployconfiguration.h"
#include "ubuntucmakebuildconfiguration.h"
#include "ubuntuconstants.h"

#include <utils/qtcassert.h>

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/deployablefile.h>

#include <remotelinux/remotelinuxdeployconfiguration.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>
#include <remotelinux/genericdirectuploadservice.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

namespace Ubuntu {
namespace Internal {

UbuntuDirectUploadStep::UbuntuDirectUploadStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, UbuntuDirectUploadStep::stepId())
    , m_deployService(new RemoteLinux::GenericDirectUploadService(this))
{
    setDefaultDisplayName(displayName());
}

UbuntuDirectUploadStep::UbuntuDirectUploadStep(ProjectExplorer::BuildStepList *bsl, UbuntuDirectUploadStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
    , m_deployService(new RemoteLinux::GenericDirectUploadService(this))
{
    setDefaultDisplayName(displayName());

    connect(target()->project(),SIGNAL(displayNameChanged()),this,SLOT(projectNameChanged()));
}

UbuntuDirectUploadStep::~UbuntuDirectUploadStep()
{
}

ProjectExplorer::BuildStepConfigWidget *UbuntuDirectUploadStep::createConfigWidget()
{
    return new ProjectExplorer::SimpleBuildStepConfigWidget(this);
}

bool UbuntuDirectUploadStep::initInternal(QString *error)
{
    projectNameChanged();
    m_deployService->setIncrementalDeployment(false);
    m_deployService->setIgnoreMissingFiles(false);
    return deployService()->isDeploymentPossible(error);
}

RemoteLinux::AbstractRemoteLinuxDeployService *UbuntuDirectUploadStep::deployService() const
{
    return m_deployService;
}

bool UbuntuDirectUploadStep::fromMap(const QVariantMap &map)
{
    if (!AbstractRemoteLinuxDeployStep::fromMap(map))
        return false;
    return true;
}

QVariantMap UbuntuDirectUploadStep::toMap() const
{
    return AbstractRemoteLinuxDeployStep::toMap();
}


Core::Id UbuntuDirectUploadStep::stepId()
{
    return Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID;
}

QString UbuntuDirectUploadStep::displayName()
{
    return tr("Upload files to Ubuntu Device");
}

void UbuntuDirectUploadStep::projectNameChanged()
{
    ProjectExplorer::DeployableFile f(target()->activeBuildConfiguration()->buildDirectory().toString()+QDir::separator()+QLatin1String("package")
                                      ,QString::fromLatin1("/home/phablet/dev_tmp/%1").arg(target()->project()->displayName()));
    m_deployService->setDeployableFiles(QList<ProjectExplorer::DeployableFile>()<<f);
    target()->project()->displayName();
}


UbuntuDeployConfigurationFactory::UbuntuDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("UbuntuDeployConfiguration"));
}

QList<Core::Id> UbuntuDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;
    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(parent->kit());
    if (!tc || tc->targetAbi().os() != ProjectExplorer::Abi::LinuxOS)
        return ids;
    const Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (devType == Constants::UBUNTU_DEVICE_TYPE_ID)
        ids << Core::Id(Constants::UBUNTU_DEPLOYCONFIGURATION_ID);
    return ids;
}

QString UbuntuDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::UBUNTU_DEPLOYCONFIGURATION_ID))
        return tr("Deploy to Ubuntu Device");
    return QString();
}

bool UbuntuDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::DeployConfiguration *UbuntuDeployConfigurationFactory::create(ProjectExplorer::Target *parent,
    const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    ProjectExplorer::DeployConfiguration * const dc
            = new RemoteLinux::RemoteLinuxDeployConfiguration(parent, id,
                                                              tr("Deploy to Ubuntu Device"));

    int step = 0;
    if(parent->project()->id() == Core::Id(CMakeProjectManager::Constants::CMAKEPROJECT_ID)){
        UbuntuCMakeMakeStep* mkStep = new UbuntuCMakeMakeStep(dc->stepList());
        mkStep->setUseNinja(false);
        mkStep->setAdditionalArguments(QLatin1String("DESTDIR=package install"));
        dc->stepList()->insertStep(0, mkStep);
        step++;
    }

    dc->stepList()->insertStep(step+1, new RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));

    UbuntuDirectUploadStep* upload = new UbuntuDirectUploadStep(dc->stepList());
    dc->stepList()->insertStep(step+2,upload);
    return dc;
}

bool UbuntuDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *UbuntuDeployConfigurationFactory::restore(ProjectExplorer::Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = ProjectExplorer::idFromMap(map);
    RemoteLinux::RemoteLinuxDeployConfiguration * const dc
            = new RemoteLinux::RemoteLinuxDeployConfiguration(parent, id,
                                                              tr("Deploy to Ubuntu Device"));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

ProjectExplorer::DeployConfiguration *UbuntuDeployConfigurationFactory::clone(ProjectExplorer::Target *parent,
    ProjectExplorer::DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new RemoteLinux::RemoteLinuxDeployConfiguration(parent
                                                           ,qobject_cast<RemoteLinux::RemoteLinuxDeployConfiguration *>(product));
}

QList<Core::Id> UbuntuDeployStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if(!canHandle(parent->target()))
        return QList<Core::Id>();
    return QList<Core::Id>()
            <<Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID
            <<RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep::stepId()
            <<Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID;
}

QString UbuntuDeployStepFactory::displayNameForId(const Core::Id id) const
{
    if(id == Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID)
        return UbuntuDirectUploadStep::displayName();
    else if(id == RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep::stepDisplayName();
    else if(id == Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID)
        return tr("Make Package Step");

    return QString();
}

bool UbuntuDeployStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if(!canHandle(parent->target()))
        return false;

    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::BuildStep *UbuntuDeployStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    if(id == Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID)
        return new UbuntuDirectUploadStep(parent);
    else if(id == RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return new RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep(parent,id);
    else if(id == Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID) {
        UbuntuCMakeMakeStep* mkStep = new UbuntuCMakeMakeStep(parent);
        mkStep->setUseNinja(false);
        mkStep->setAdditionalArguments(QLatin1String("DESTDIR=package install"));
        return mkStep;
    }
    return 0;
}

bool UbuntuDeployStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent,ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *UbuntuDeployStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    Core::Id id = ProjectExplorer::idFromMap(map);
    if(!canCreate(parent,id))
        return 0;

    ProjectExplorer::BuildStep* step = create(parent,id);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }

    return step;
}

bool UbuntuDeployStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent,product->id());
}

ProjectExplorer::BuildStep *UbuntuDeployStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;

    const Core::Id id = product->id();
    if(id == Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID)
        return new UbuntuDirectUploadStep(parent, static_cast<UbuntuDirectUploadStep *>(product));
    else if(id == RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return new RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep(parent
                                             ,static_cast<RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep *>(product));
    else if(id == Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID) {
         return new UbuntuCMakeMakeStep(parent, static_cast<UbuntuCMakeMakeStep *>(product));
    }
    return 0;
}

bool UbuntuDeployStepFactory::canHandle(const ProjectExplorer::Target *t) const
{
    QTC_ASSERT(t, return false);
    if (!t->project()->supportsKit(t->kit()))
        return false;

    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(t->kit());
    if(!tc || tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
        return false;

    return t->project()->id() == Core::Id(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
}

} // namespace Internal
} // namespace Ubuntu
