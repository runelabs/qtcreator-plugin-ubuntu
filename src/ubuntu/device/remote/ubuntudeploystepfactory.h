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
    virtual QList<ProjectExplorer::BuildStepInfo> availableSteps(ProjectExplorer::BuildStepList *parent) const override;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id) override;
    virtual ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
    virtual ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) override;

private:
    bool canHandle(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEPLOYSTEPFACTORY_H
