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
#include "ubuntucmakemakestep.h"
#include "ubuntuconstants.h"
#include "ubuntuprojecthelper.h"
#include "clicktoolchain.h"
#include "ubuntupackagestep.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/customparser.h>
#include <cmakeprojectmanager/cmakeproject.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <utils/qtcassert.h>

#include <QTimer>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuCMakeMakeStepFactory
 * Factory class to create UbuntuCMakeMakeStep
 * build steps
 */
QList<Core::Id> UbuntuCMakeMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if(!canHandle(parent->target()))
        return QList<Core::Id>();

    return QList<Core::Id>() << Core::Id(Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID);
}

/*!
 * \brief UbuntuCMakeBuildConfigurationFactory::canHandle
 * checks if we can create buildconfigurations for the given target
 */
bool UbuntuCMakeMakeStepFactory::canHandle(const ProjectExplorer::Target *t) const
{
    QTC_ASSERT(t, return false);
    if (!t->project()->supportsKit(t->kit()))
        return false;

    if(ProjectExplorer::DeviceKitInformation::deviceId(t->kit()) != ProjectExplorer::Constants::DESKTOP_DEVICE_ID) {
        ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(t->kit());
        if(!tc || tc->type() != QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID))
            return false;
    }

    return t->project()->id() == Core::Id(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
}

QString UbuntuCMakeMakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID)
        return tr("UbuntuSDK-Make", "Display name for UbuntuCMakeMakeStep id.");
    return QString();
}

bool UbuntuCMakeMakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if (canHandle(parent->target()))
        return availableCreationIds(parent).contains(id);
    return false;
}

ProjectExplorer::BuildStep *UbuntuCMakeMakeStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    UbuntuCMakeMakeStep *step = new UbuntuCMakeMakeStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setUseNinja(false);
        step->setClean(true);
        step->setAdditionalArguments(QLatin1String("clean"));
    }
    return step;
}

bool UbuntuCMakeMakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    Core::Id toRestore = ProjectExplorer::idFromMap(map);
    return canCreate(parent, toRestore);
}

ProjectExplorer::BuildStep *UbuntuCMakeMakeStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Core::Id toRestore = ProjectExplorer::idFromMap(map);
    ProjectExplorer::BuildStep* step = create(parent,toRestore);
    if(step->fromMap(map))
        return step;

    delete step;
    return 0;
}

bool UbuntuCMakeMakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *UbuntuCMakeMakeStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;

    if(product->id() == Core::Id(Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID))
        return new UbuntuCMakeMakeStep(parent, static_cast<UbuntuCMakeMakeStep *>(product));

    QTC_ASSERT(false,return 0);
}

/*!
 * \class UbuntuCMakeMakeStep
 * Represents a make or make clean call in the Ubuntu-SDK build chain
 */
UbuntuCMakeMakeStep::UbuntuCMakeMakeStep(ProjectExplorer::BuildStepList *bsl)
    : MakeStep(bsl,Core::Id(Constants::UBUNTU_CLICK_CMAKE_MAKESTEP_ID))
{
    setDefaultDisplayName(tr("Ubuntu SDK Make"));
}

UbuntuCMakeMakeStep::UbuntuCMakeMakeStep(ProjectExplorer::BuildStepList *bsl, UbuntuCMakeMakeStep *bs)
    : MakeStep(bsl,bs)
{
}

UbuntuCMakeMakeStep::~UbuntuCMakeMakeStep()
{

}

QString UbuntuCMakeMakeStep::makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const
{
    if (tc)
        return tc->makeCommand(env);
    return QString::fromLatin1(Constants::UBUNTU_CLICK_MAKE_WRAPPER).arg(Constants::UBUNTU_SCRIPTPATH);
}

} // namespace Internal
} // namespace Ubuntu
