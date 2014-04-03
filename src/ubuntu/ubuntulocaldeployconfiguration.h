#ifndef UBUNTU_INTERNAL_UBUNTULOCALDEPLOYCONFIGURATIONFACTORY_H
#define UBUNTU_INTERNAL_UBUNTULOCALDEPLOYCONFIGURATIONFACTORY_H

#include <projectexplorer/deployconfiguration.h>

namespace Ubuntu {
namespace Internal {

class UbuntuLocalDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit UbuntuLocalDeployConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;
    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *product);
};

class UbuntuLocalDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    friend class UbuntuLocalDeployConfigurationFactory; // for the ctors
protected:
    UbuntuLocalDeployConfiguration(ProjectExplorer::Target *target, const Core::Id id);
    UbuntuLocalDeployConfiguration(ProjectExplorer::Target *target, UbuntuLocalDeployConfiguration *source);

protected slots:
    void selectAsDefaultHack();
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTULOCALDEPLOYCONFIGURATIONFACTORY_H
