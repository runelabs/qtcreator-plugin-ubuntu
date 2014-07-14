#ifndef UBUNTU_INTERNAL_UBUNTUDEPLOYSTEPFACTORY_H
#define UBUNTU_INTERNAL_UBUNTUDEPLOYSTEPFACTORY_H

#include <projectexplorer/buildstep.h>
#include <projectexplorer/abstractprocessstep.h>

namespace Ubuntu {
namespace Internal {

class UbuntuDeployStepFactory : public ProjectExplorer::IBuildStepFactory
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
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEPLOYSTEPFACTORY_H
