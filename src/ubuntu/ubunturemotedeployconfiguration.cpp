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
#include "ubunturemotedeployconfiguration.h"
#include "ubuntucmakebuildconfiguration.h"
#include "ubuntucmakemakestep.h"
#include "ubuntuconstants.h"
#include "ubuntuprojectguesser.h"

#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>
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

#include <QDir>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuRemoteDeployConfiguration::UbuntuRemoteDeployConfiguration(ProjectExplorer::Target *target)
    : RemoteLinux::RemoteLinuxDeployConfiguration(target,Constants::UBUNTU_DEPLOYCONFIGURATION_ID,QString())
{
    setDefaultDisplayName(tr("Deploy to Ubuntu Device"));
}

UbuntuRemoteDeployConfiguration::UbuntuRemoteDeployConfiguration(ProjectExplorer::Target *target, UbuntuRemoteDeployConfiguration *source) :
    RemoteLinux::RemoteLinuxDeployConfiguration(target,source)
{

}

ProjectExplorer::NamedWidget *UbuntuRemoteDeployConfiguration::createConfigWidget()
{
    return new ProjectExplorer::NamedWidget();
}

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

static void createFileList(const QDir &rootDir
                           , const QString& subDir
                           , const QString& remoteRoot
                           , QList<ProjectExplorer::DeployableFile>* list)
{
    QDir currentDir(rootDir.absolutePath()+subDir);
    if(!currentDir.exists())
        return;

    QFileInfoList entries = currentDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &entry, entries) {
        if(entry.isDir()) {
            QString newSub = (subDir.endsWith(QLatin1String("/"))) ? (QString)(subDir+entry.fileName()) : (QString)(subDir+(QLatin1String("/")+entry.fileName()));
            createFileList(rootDir,newSub,remoteRoot,list);
        } else {
            QString targetPath = QDir::cleanPath(remoteRoot
                    + subDir
                    + QLatin1String("/"));

            if(debug) qDebug()<<"Uploading: "<<entry.fileName();
            if(debug) qDebug()<<" to "<<targetPath;
            list->append(ProjectExplorer::DeployableFile(entry.absoluteFilePath()
                                                         , targetPath
                                                         , entry.isExecutable() ? ProjectExplorer::DeployableFile::TypeExecutable
                                                                                : ProjectExplorer::DeployableFile::TypeNormal));
        }
    }
}

void UbuntuDirectUploadStep::projectNameChanged()
{
    if(debug) qDebug()<<"------------------------ Updating DEPLOYLIST ---------------------------";
    //iterate over the .deploy dir and put all files in the list
    QDir d(target()->activeBuildConfiguration()->buildDirectory().toString()+QDir::separator()+QLatin1String(Constants::UBUNTU_DEPLOY_DESTDIR));
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


UbuntuRemoteDeployConfigurationFactory::UbuntuRemoteDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("UbuntuDeployConfiguration"));
}

QList<Core::Id> UbuntuRemoteDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;

    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(parent->kit());
    if (!tc || tc->targetAbi().os() != ProjectExplorer::Abi::LinuxOS)
        return ids;

    //for now only support cmake projects
    if(parent->project()->id() != CMakeProjectManager::Constants::CMAKEPROJECT_ID)
        return ids;

    const Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (devType == Constants::UBUNTU_DEVICE_TYPE_ID && !UbuntuProjectGuesser::isScopesProject(parent->project()))
        ids << Core::Id(Constants::UBUNTU_DEPLOYCONFIGURATION_ID);

    return ids;
}

QString UbuntuRemoteDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(Constants::UBUNTU_DEPLOYCONFIGURATION_ID))
        return tr("Deploy to Ubuntu Device");
    return QString();
}

bool UbuntuRemoteDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::DeployConfiguration *UbuntuRemoteDeployConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                               const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    ProjectExplorer::DeployConfiguration * const dc
            = new UbuntuRemoteDeployConfiguration(parent);

    int step = 0;
    if(parent->project()->id() == Core::Id(CMakeProjectManager::Constants::CMAKEPROJECT_ID)){
        UbuntuPackageStep *pckStep = new UbuntuPackageStep(dc->stepList());
        dc->stepList()->insertStep(0,pckStep);
    }

    RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep *checkSpace = new RemoteLinux::RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList());
    checkSpace->setPathToCheck(QStringLiteral("/tmp"));
    dc->stepList()->insertStep(step+1, checkSpace);

    UbuntuDirectUploadStep* upload = new UbuntuDirectUploadStep(dc->stepList());
    dc->stepList()->insertStep(step+2,upload);
    return dc;
}

bool UbuntuRemoteDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *UbuntuRemoteDeployConfigurationFactory::restore(ProjectExplorer::Target *parent,
                                                                                const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    RemoteLinux::RemoteLinuxDeployConfiguration * const dc
            = new UbuntuRemoteDeployConfiguration(parent);

    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

ProjectExplorer::DeployConfiguration *UbuntuRemoteDeployConfigurationFactory::clone(ProjectExplorer::Target *parent,
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
            <<Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID;
}

QString UbuntuDeployStepFactory::displayNameForId(const Core::Id id) const
{
    if(id == Constants::UBUNTU_DEPLOY_UPLOADSTEP_ID)
        return UbuntuDirectUploadStep::displayName();
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

    return (t->project()->id() == Core::Id(CMakeProjectManager::Constants::CMAKEPROJECT_ID)
            && !UbuntuProjectGuesser::isScopesProject(t->project()));
}

} // namespace Internal
} // namespace Ubuntu
