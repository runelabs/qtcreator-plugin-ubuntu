#ifndef UBUNTULOCALRUNCONTROLFACTORY_H
#define UBUNTULOCALRUNCONTROLFACTORY_H

#include <projectexplorer/runconfiguration.h>
#include <utils/environment.h>

namespace Ubuntu {
namespace Internal {

class UbuntuLocalRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT
public:
    explicit UbuntuLocalRunControlFactory() {}
    virtual ~UbuntuLocalRunControlFactory() {}

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, Core::Id mode) const override;
    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                                        Core::Id mode, QString *errorMessage) override;
    QString displayName() const;

};

} //namespace Internal
} //namespace Ubuntu
#endif // UBUNTULOCALRUNCONTROLFACTORY_H
