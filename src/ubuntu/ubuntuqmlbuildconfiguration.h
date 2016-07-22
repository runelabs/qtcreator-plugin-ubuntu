#ifndef UBUNTU_INTERNAL_UBUNTUQMLBUILDCONFIGURATION_H
#define UBUNTU_INTERNAL_UBUNTUQMLBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <utils/pathchooser.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/abstractprocessstep.h>

namespace ProjectExplorer {
    class ToolChain;
}

namespace Ubuntu {
namespace Internal {

class UbuntuQmlBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    UbuntuQmlBuildConfiguration(ProjectExplorer::Target *target);
    UbuntuQmlBuildConfiguration(ProjectExplorer::Target *target, UbuntuQmlBuildConfiguration *source);

    // ProjectConfiguration interface
    virtual bool fromMap(const QVariantMap &map) override;
    virtual QVariantMap toMap() const override;

    // BuildConfiguration interface
    virtual ProjectExplorer::NamedWidget *createConfigWidget() override;
    virtual BuildType buildType() const override;

private:
    friend class UbuntuQmlBuildConfigurationFactory;
};

class UbuntuQmlBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    explicit UbuntuQmlBuildSettingsWidget(UbuntuQmlBuildConfiguration *conf, QWidget *parent = 0);

private slots:
    void updateBuildDirectory () const;

private:
    Utils::PathChooser *m_pathChooser;
    UbuntuQmlBuildConfiguration *m_buildConfiguration;
};

class UbuntuQmlUpdateTranslationTemplateStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    UbuntuQmlUpdateTranslationTemplateStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuQmlUpdateTranslationTemplateStep(ProjectExplorer::BuildStepList *bsl, Core::Id typeId);
    UbuntuQmlUpdateTranslationTemplateStep(ProjectExplorer::BuildStepList *bsl, UbuntuQmlUpdateTranslationTemplateStep *bs);

    // BuildStep interface
    virtual bool init() override;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const;
};

class UbuntuQmlBuildTranslationStep : public UbuntuQmlUpdateTranslationTemplateStep
{
    Q_OBJECT
public:
    UbuntuQmlBuildTranslationStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuQmlBuildTranslationStep(ProjectExplorer::BuildStepList *bsl, UbuntuQmlBuildTranslationStep *bs);

    // BuildStep interface
    virtual bool init() override;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    void run(QFutureInterface<bool> &fi) override;

private:
    QString m_translationDir;
};

class UbuntuQmlBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT
public:
    UbuntuQmlBuildConfigurationFactory(QObject *parent = 0);
    ~UbuntuQmlBuildConfigurationFactory();

    // IBuildConfigurationFactory interface
    virtual int priority(const ProjectExplorer::Target *parent) const override;
    virtual QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    virtual int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    virtual QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    virtual UbuntuQmlBuildConfiguration *create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const override;
    virtual bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    virtual UbuntuQmlBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
    virtual bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const override;
    virtual UbuntuQmlBuildConfiguration*clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) override;
private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    QList<ProjectExplorer::BuildInfo *> createBuildInfos (const ProjectExplorer::Kit *k, const QString &projectDir) const;
};

class UbuntuQmlBuildStepFactory : public ProjectExplorer::IBuildStepFactory
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

#endif // UBUNTU_INTERNAL_UBUNTUQMLBUILDCONFIGURATION_H
