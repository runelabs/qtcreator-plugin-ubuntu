#ifndef UBUNTU_INTERNAL_UBUNTUCMAKEBUILDCONFIGURATION_H
#define UBUNTU_INTERNAL_UBUNTUCMAKEBUILDCONFIGURATION_H

#include <cmakeprojectmanager/cmakebuildconfiguration.h>
#include <cmakeprojectmanager/makestep.h>
#include <projectexplorer/namedwidget.h>


namespace Utils{
    class PathChooser;
}
namespace CMakeProjectManager{
    class ArgumentsLineEdit;
}

namespace Ubuntu {
namespace Internal {

class UbuntuCMakeBuildConfiguration : public CMakeProjectManager::CMakeBuildConfiguration
{
    Q_OBJECT
public:
    UbuntuCMakeBuildConfiguration(ProjectExplorer::Target *parent);
    virtual ProjectExplorer::NamedWidget *createConfigWidget();
protected:
    UbuntuCMakeBuildConfiguration(ProjectExplorer::Target *parent, UbuntuCMakeBuildConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

    friend class UbuntuCMakeBuildConfigurationFactory;
};

class UbuntuCMakeBuildConfigurationFactory : public CMakeProjectManager::CMakeBuildConfigurationFactory
{
    Q_OBJECT
public:
    UbuntuCMakeBuildConfigurationFactory(QObject *parent = 0);
    ~UbuntuCMakeBuildConfigurationFactory();

    // IBuildConfigurationFactory interface
    virtual int priority(const ProjectExplorer::Target *parent) const;
    virtual QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const;
    virtual int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const;
    virtual QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const;
    virtual UbuntuCMakeBuildConfiguration *create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const;
    virtual bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    virtual UbuntuCMakeBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    virtual bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const;
    virtual UbuntuCMakeBuildConfiguration*clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product);
private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

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
    virtual ~UbuntuCMakeMakeStep();

    virtual QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const;

protected:
    UbuntuCMakeMakeStep(ProjectExplorer::BuildStepList *bsl, UbuntuCMakeMakeStep *bs);

    friend class UbuntuCMakeMakeStepFactory;
};

class UbuntuCMakeBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT
public:
    UbuntuCMakeBuildSettingsWidget(UbuntuCMakeBuildConfiguration *bc);

private slots:
    void onArgumentsChanged();
    void onBuilddirChanged();
private:
    Utils::PathChooser *m_pathChooser;
    CMakeProjectManager::ArgumentsLineEdit *m_userArguments;
    UbuntuCMakeBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCMAKEBUILDCONFIGURATION_H
