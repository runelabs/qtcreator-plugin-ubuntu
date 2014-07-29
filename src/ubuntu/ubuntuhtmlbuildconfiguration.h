#ifndef UBUNTU_INTERNAL_UBUNTUHTMLBUILDCONFIGURATION_H
#define UBUNTU_INTERNAL_UBUNTUHTMLBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <utils/pathchooser.h>
#include <projectexplorer/namedwidget.h>

namespace Ubuntu {
namespace Internal {

class UbuntuHtmlBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    UbuntuHtmlBuildConfiguration(ProjectExplorer::Target *target);
    UbuntuHtmlBuildConfiguration(ProjectExplorer::Target *target, UbuntuHtmlBuildConfiguration *source);

    // ProjectConfiguration interface
    virtual bool fromMap(const QVariantMap &map) override;
    virtual QVariantMap toMap() const override;

    // BuildConfiguration interface
    virtual ProjectExplorer::NamedWidget *createConfigWidget() override;
    virtual BuildType buildType() const override;

private:
    friend class UbuntuHtmlBuildConfigurationFactory;
};

class UbuntuHtmlBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    explicit UbuntuHtmlBuildSettingsWidget(UbuntuHtmlBuildConfiguration *conf, QWidget *parent = 0);

private slots:
    void updateBuildDirectory () const;

private:
    Utils::PathChooser *m_pathChooser;
    UbuntuHtmlBuildConfiguration *m_buildConfiguration;
};

class UbuntuHtmlBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT
public:
    UbuntuHtmlBuildConfigurationFactory(QObject *parent = 0);
    ~UbuntuHtmlBuildConfigurationFactory();

    // IBuildConfigurationFactory interface
    virtual int priority(const ProjectExplorer::Target *parent) const override;
    virtual QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    virtual int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    virtual QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    virtual UbuntuHtmlBuildConfiguration *create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const override;
    virtual bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    virtual UbuntuHtmlBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
    virtual bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const override;
    virtual UbuntuHtmlBuildConfiguration*clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) override;
private:
    bool canHandle(const ProjectExplorer::Target *t) const;
    QList<ProjectExplorer::BuildInfo *> createBuildInfos (const ProjectExplorer::Kit *k, const QString &projectDir) const;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUHTMLBUILDCONFIGURATION_H
