#include "ubuntudirectuploadstep.h"
#include "ubuntupackagestep.h"
#include "ubuntuconstants.h"

#include <utils/qtcassert.h>
#include <projectexplorer/deployablefile.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>

#include <remotelinux/genericdirectuploadservice.h>
#include <remotelinux/remotelinuxdeployconfiguration.h>

#include <QFileInfo>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

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
    connect(target(),SIGNAL(applicationTargetsChanged()),this,SLOT(projectNameChanged()));
}

UbuntuDirectUploadStep::~UbuntuDirectUploadStep()
{
}

void UbuntuDirectUploadStep::run(QFutureInterface<bool> &fi)
{
    m_foundClickPackage = false;
    projectNameChanged();
    if(!m_foundClickPackage) {
        emit addOutput(tr("Deploy step failed. No click package was created"), ErrorMessageOutput);
        fi.reportResult(false);
        emit finished();
        return;
    }


    m_deployService->setIncrementalDeployment(false);
    m_deployService->setIgnoreMissingFiles(false);

    QString whyNot;
    if(!deployService()->isDeploymentPossible(&whyNot)) {
        emit addOutput(tr("Deploy step failed. %1").arg(whyNot), ErrorMessageOutput);
        fi.reportResult(false);
        emit finished();
        return;
    }

    AbstractRemoteLinuxDeployStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget *UbuntuDirectUploadStep::createConfigWidget()
{
    return new ProjectExplorer::SimpleBuildStepConfigWidget(this);
}

bool UbuntuDirectUploadStep::initInternal(QString *error)
{
    Q_UNUSED(error)
    return true;
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
    if(debug) qDebug()<<"------------------------ Updating DEPLOYLIST ---------------------------";

    ProjectExplorer::BuildStepList *bsList = deployConfiguration()->stepList();

    QList<ProjectExplorer::DeployableFile> list;
    foreach(ProjectExplorer::BuildStep *currStep ,bsList->steps()) {
        UbuntuPackageStep *pckStep = qobject_cast<UbuntuPackageStep*>(currStep);
        if(!pckStep)
            continue;

        QFileInfo info(pckStep->packagePath());
        if(info.exists()) {
            list.append(ProjectExplorer::DeployableFile(info.filePath(),
                                                        QStringLiteral("/tmp")));

            list.append(ProjectExplorer::DeployableFile(QStringLiteral("%1/qtc_device_applaunch.py").arg(Constants::UBUNTU_SCRIPTPATH),
                                                        QStringLiteral("/tmp")));
            m_deployService->setDeployableFiles(list);
            m_foundClickPackage = true;
            break;
        }
    }
}
} // namespace Internal
} // namespace Ubuntu
