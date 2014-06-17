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
#ifndef UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H
#define UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H

#include <projectexplorer/buildstep.h>
#include <projectexplorer/abstractprocessstep.h>
#include <cmakeprojectmanager/makestep.h>

namespace Ubuntu {
namespace Internal {

class UbuntuCMakeMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    // IBuildStepFactory interface
    virtual QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const override;
    virtual QString displayNameForId(const Core::Id id) const override;
    virtual bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const override;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id) override;
    virtual bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const override;
    virtual ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
    virtual bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const override;
    virtual ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) override;

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

class UbuntuCMakeMakeStep : public CMakeProjectManager::MakeStep
{
    Q_OBJECT

public:
    UbuntuCMakeMakeStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuCMakeMakeStep(ProjectExplorer::BuildStepList *bsl, UbuntuCMakeMakeStep *bs);
    virtual ~UbuntuCMakeMakeStep();

    virtual QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const override;

    friend class UbuntuCMakeMakeStepFactory;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H
