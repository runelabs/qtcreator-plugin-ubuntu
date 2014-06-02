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
#include "ubuntucmakebuildconfiguration.h"
#include "ubuntuconstants.h"
#include "clicktoolchain.h"

#include <coreplugin/mimedatabase.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectconfiguration.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <cmakeprojectmanager/cmakebuildinfo.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/argumentslineedit.h>
#include <cmakeprojectmanager/generatorinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QLabel>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

/*!
 * \class UbuntuCMakeBuildConfiguration
 * Represents the build configuration for a Ubuntu-SDK
 * CMake Project
 */

UbuntuCMakeBuildConfiguration::UbuntuCMakeBuildConfiguration(ProjectExplorer::Target *parent)
    :CMakeBuildConfiguration(parent,Core::Id(Constants::UBUNTU_CLICK_CMAKE_BC_ID))
{
}

ProjectExplorer::NamedWidget *UbuntuCMakeBuildConfiguration::createConfigWidget()
{
    return new UbuntuCMakeBuildSettingsWidget(this);
}

UbuntuCMakeBuildConfiguration::UbuntuCMakeBuildConfiguration(ProjectExplorer::Target *parent, UbuntuCMakeBuildConfiguration *source)
    : CMakeBuildConfiguration(parent,source)
{
}

bool UbuntuCMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if(!CMakeBuildConfiguration::fromMap(map))
        return false;

    this->setUseNinja(false);
    return true;
}

/*!
 * \class UbuntuCMakeBuildConfigurationFactory
 * Factory class to create UbuntuCMakeBuildConfiguration
 * instances.
 */

UbuntuCMakeBuildConfigurationFactory::UbuntuCMakeBuildConfigurationFactory(QObject *parent)
    : CMakeBuildConfigurationFactory(parent)
{
}

UbuntuCMakeBuildConfigurationFactory::~UbuntuCMakeBuildConfigurationFactory()
{
}

int UbuntuCMakeBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    if(canHandle(parent))
        return 10;
    return -1;
}

QList<ProjectExplorer::BuildInfo *> UbuntuCMakeBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target *parent) const
{
    if(!canHandle(parent))
        QList<ProjectExplorer::BuildInfo *>();

    QList<ProjectExplorer::BuildInfo *> infos = CMakeBuildConfigurationFactory::availableBuilds(parent);
    foreach(ProjectExplorer::BuildInfo* info, infos)
        info->typeName = tr("Ubuntu SDK Build");

    return infos;
}

int UbuntuCMakeBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    if(!k)
        return false;

    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if(!tc || tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
        return false;

    return (Core::MimeDatabase::findByFile(QFileInfo(projectPath)).matchesType(QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE))) ? 10 : -1;
}

QList<ProjectExplorer::BuildInfo *> UbuntuCMakeBuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    QList<ProjectExplorer::BuildInfo *> infos = CMakeBuildConfigurationFactory::availableSetups(k,projectPath);
    foreach(ProjectExplorer::BuildInfo* info, infos)
        info->typeName = tr("Ubuntu SDK Build");

    return infos;
}

UbuntuCMakeBuildConfiguration *UbuntuCMakeBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);
    QTC_ASSERT(canHandle(parent),return 0);


    CMakeProjectManager::CMakeBuildInfo copy(*static_cast<const CMakeProjectManager::CMakeBuildInfo *>(info));

    CMakeProjectManager::CMakeProject *project = static_cast<CMakeProjectManager::CMakeProject *>(parent->project());

    if (copy.buildDirectory.isEmpty())
        copy.buildDirectory
                = Utils::FileName::fromString(project->shadowBuildDirectory(project->projectFilePath(),
                                                                            parent->kit(),
                                                                            copy.displayName));

    UbuntuCMakeBuildConfiguration *bc = new UbuntuCMakeBuildConfiguration(parent);
    bc->setDisplayName(copy.displayName);
    bc->setDefaultDisplayName(copy.displayName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    UbuntuCMakeMakeStep *makeStep = new UbuntuCMakeMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    UbuntuCMakeMakeStep *cleanMakeStep = new UbuntuCMakeMakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setAdditionalArguments(QLatin1String("clean"));
    cleanMakeStep->setClean(true);

    bc->setBuildDirectory(Utils::FileName::fromString(copy.buildDirectory.toString()));
    bc->setUseNinja(false);

    // Default to all
    if (project->hasBuildTarget(QLatin1String("all")))
        makeStep->setBuildTarget(QLatin1String("all"), true);

    return bc;
}

bool UbuntuCMakeBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == Constants::UBUNTU_CLICK_CMAKE_BC_ID;
}

UbuntuCMakeBuildConfiguration *UbuntuCMakeBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    UbuntuCMakeBuildConfiguration *bc = new UbuntuCMakeBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool UbuntuCMakeBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const
{
    if (!canHandle(parent))
        return false;

    return product->id() == Constants::UBUNTU_CLICK_CMAKE_BC_ID;
}

UbuntuCMakeBuildConfiguration *UbuntuCMakeBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    UbuntuCMakeBuildConfiguration *old = static_cast<UbuntuCMakeBuildConfiguration *>(product);
    return new UbuntuCMakeBuildConfiguration(parent, old);
}

/*!
 * \brief UbuntuCMakeBuildConfigurationFactory::canHandle
 * checks if we can create buildconfigurations for the given target
 */
bool UbuntuCMakeBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    QTC_ASSERT(t, return false);
    if (!t->project()->supportsKit(t->kit()))
        return false;

    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(t->kit());
    if(!tc || tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
        return false;

    return t->project()->id() == Core::Id(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
}

UbuntuCMakeBuildSettingsWidget::UbuntuCMakeBuildSettingsWidget(UbuntuCMakeBuildConfiguration *bc) : m_buildConfiguration(bc)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(20, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    bool inSource = CMakeProjectManager::CMakeProject::hasInSourceBuild(bc->target()->project()->projectDirectory());
    if(inSource) {
        QLabel* inSourceLabel = new QLabel(this);
        inSourceLabel->setWordWrap(true);
        inSourceLabel->setText(tr("Qt Creator has detected an <b>in-source-build in %1</b> "
                                  "which prevents shadow builds. Qt Creator will not allow you to change the build directory. "
                                  "If you want a shadow build, clean your source directory and re-open the project.")
                               .arg(bc->target()->project()->projectDirectory()));
        fl->addRow(inSourceLabel);
    }

    if(!inSource) {
        m_pathChooser = new Utils::PathChooser(this);
        m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
        fl->addRow(tr("Build directory:"), m_pathChooser);

        connect(m_pathChooser->lineEdit(),SIGNAL(editingFinished()),this,SLOT(onBuilddirChanged()));
    }

    m_userArguments = new CMakeProjectManager::ArgumentsLineEdit(this);
    fl->addRow(tr("CMake arguments:"),m_userArguments);
    m_userArguments->setText(Utils::QtcProcess::joinArgs(m_buildConfiguration->arguments()));
    //m_userArguments->setHistoryCompleter(QLatin1String("CMakeArgumentsLineEdit"));
    connect(m_userArguments,SIGNAL(editingFinished()),this,SLOT(onArgumentsChanged()));

    setDisplayName(tr("Ubuntu SDK CMake"));
}

void UbuntuCMakeBuildSettingsWidget::onArgumentsChanged()
{
    if(!m_userArguments->isValid())
        return;

    QStringList args = Utils::QtcProcess::splitArgs(m_userArguments->text());

    if(m_buildConfiguration->arguments() != args)
        m_buildConfiguration->setArguments(args);
}

void UbuntuCMakeBuildSettingsWidget::onBuilddirChanged()
{
    if(debug) qDebug()<<"Changing builddir to: "<<m_pathChooser->fileName().toString();
    m_buildConfiguration->setBuildDirectory(m_pathChooser->fileName());
}

} // namespace Internal
} // namespace Ubuntu
