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
    virtual QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    virtual QString displayNameForId(const Core::Id id) const;
    virtual bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    virtual bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    virtual bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    virtual ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);

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

    virtual QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const;

    friend class UbuntuCMakeMakeStepFactory;
};

class UbuntuCMakeDeployStep : public CMakeProjectManager::MakeStep
{
    Q_OBJECT

public:
    UbuntuCMakeDeployStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuCMakeDeployStep(ProjectExplorer::BuildStepList *bsl, UbuntuCMakeDeployStep *bs);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual ~UbuntuCMakeDeployStep();

protected:
    virtual bool fromMap(const QVariantMap &map);

    friend class UbuntuCMakeMakeStepFactory;
};

class UbuntuClickPackageStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    UbuntuClickPackageStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuClickPackageStep(ProjectExplorer::BuildStepList *bsl, UbuntuClickPackageStep *bs);

    virtual ~UbuntuClickPackageStep();

    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    QString packagePath () const;
protected:
    // AbstractProcessStep interface
    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);
    virtual void processFinished(int exitCode, QProcess::ExitStatus status);

private:
    QList<ProjectExplorer::Task> m_tasks;
    QString    m_lastLine;
    QString    m_clickPackageName;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H
