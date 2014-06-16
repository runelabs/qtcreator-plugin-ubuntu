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

class UbuntuPackageStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:

    enum State {
        Idle,
        MakeInstall,
        PreparePackage,
        ClickBuild
    };

    UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl, UbuntuPackageStep *other);
    virtual ~UbuntuPackageStep();

public:
    // BuildStep interface
    virtual bool init() override;
    virtual void run(QFutureInterface<bool> &fi) override;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    virtual bool immutable() const override;
    virtual bool runInGuiThread() const override;

    // ProjectConfiguration interface
    virtual bool fromMap(const QVariantMap &map) override;
    virtual QVariantMap toMap() const override;

    QString packagePath () const;

protected:
    void setupAndStartProcess ( const ProjectExplorer::ProcessParameters &params );
    bool processFinished ();
    void cleanup ();
    void stdOutput ( const QString &line );
    void stdError  ( const QString &line );
    QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const;

protected slots:
    void doNextStep ();
    void injectDebugHelperStep ();

    void onProcessStdOut ();
    void onProcessStdErr ();
    void onProcessFailedToStart ();

    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded (const ProjectExplorer::Task & task);

private:
    State m_state;
    QString m_lastLine;
    QString m_clickPackageName;
    QList<ProjectExplorer::Task> m_tasks;
    QFutureInterface<bool> *m_futureInterface;

    ProjectExplorer::ProcessParameters m_MakeParam;
    ProjectExplorer::ProcessParameters m_ClickParam;

    Utils::QtcProcess *m_process;
    ProjectExplorer::IOutputParser *m_outputParserChain;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H
