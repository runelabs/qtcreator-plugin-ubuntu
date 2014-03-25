#ifndef UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H
#define UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H

#include <projectexplorer/buildstep.h>
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

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCMAKEMAKESTEP_H
