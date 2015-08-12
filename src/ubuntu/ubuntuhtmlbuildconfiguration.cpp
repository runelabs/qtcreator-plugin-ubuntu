#include "ubuntuhtmlbuildconfiguration.h"
#include "ubuntuconstants.h"
#include "ubuntuproject.h"

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <utils/fancylineedit.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QFormLayout>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuHtmlBuildConfiguration
 *
 * Even though HTML projects don't need to be built, we need
 * that BuildConfiguration to store the Builddirectory (where the click
 * package goes) and to enable the Target page to show us some actual
 * targets
 */

UbuntuHtmlBuildConfiguration::UbuntuHtmlBuildConfiguration(ProjectExplorer::Target *target)
    : BuildConfiguration(target,Constants::UBUNTU_CLICK_HTML_BC_ID)
{
}

UbuntuHtmlBuildConfiguration::UbuntuHtmlBuildConfiguration(ProjectExplorer::Target *target, UbuntuHtmlBuildConfiguration *source)
    : BuildConfiguration(target,source)
{

}

bool UbuntuHtmlBuildConfiguration::fromMap(const QVariantMap &map)
{
    return BuildConfiguration::fromMap(map);
}

QVariantMap UbuntuHtmlBuildConfiguration::toMap() const
{
    return BuildConfiguration::toMap();
}

ProjectExplorer::NamedWidget *UbuntuHtmlBuildConfiguration::createConfigWidget()
{
    return new UbuntuHtmlBuildSettingsWidget(this);
}

ProjectExplorer::BuildConfiguration::BuildType UbuntuHtmlBuildConfiguration::buildType() const
{
    return Release;
}

UbuntuHtmlBuildConfigurationFactory::UbuntuHtmlBuildConfigurationFactory(QObject *parent)
    : IBuildConfigurationFactory(parent)
{

}

UbuntuHtmlBuildConfigurationFactory::~UbuntuHtmlBuildConfigurationFactory()
{

}

int UbuntuHtmlBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    if (canHandle(parent))
        return 100;
    return -1;
}

QList<ProjectExplorer::BuildInfo *> UbuntuHtmlBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target *parent) const
{
    if(!canHandle(parent))
        return QList<ProjectExplorer::BuildInfo *>();
    return createBuildInfos(parent->kit(),parent->project()->projectFilePath().toString());
}

int UbuntuHtmlBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    return (k && Utils::MimeDatabase().mimeTypeForFile(projectPath)
            .matchesName(QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE))) ? 100 : -1;
}

QList<ProjectExplorer::BuildInfo *> UbuntuHtmlBuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    UbuntuKitMatcher m;
    if(priority(k,projectPath) < 0 || !m.matches(k))
        return QList<ProjectExplorer::BuildInfo *>();

    return createBuildInfos(k,projectPath);
}

UbuntuHtmlBuildConfiguration *UbuntuHtmlBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    UbuntuHtmlBuildConfiguration *conf = new UbuntuHtmlBuildConfiguration(parent);
    conf->setBuildDirectory(info->buildDirectory);
    conf->setDefaultDisplayName(info->displayName);
    conf->setDisplayName(info->displayName);

    return conf;
}

bool UbuntuHtmlBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == Constants::UBUNTU_CLICK_HTML_BC_ID;
}

UbuntuHtmlBuildConfiguration *UbuntuHtmlBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent,map) )
        return 0;

    UbuntuHtmlBuildConfiguration *conf = new UbuntuHtmlBuildConfiguration(parent);
    if (conf->fromMap(map))
        return conf;

    delete conf;
    return 0;
}

bool UbuntuHtmlBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const
{
    if (!canHandle(parent))
        return false;
    if (product->id() != Constants::UBUNTU_CLICK_HTML_BC_ID )
        return false;

    return true;
}

UbuntuHtmlBuildConfiguration *UbuntuHtmlBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product)
{
    if (!canClone(parent,product))
        return 0;
    return new UbuntuHtmlBuildConfiguration(parent,static_cast<UbuntuHtmlBuildConfiguration*>(product));
}

bool UbuntuHtmlBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    UbuntuKitMatcher m;
    if(!m.matches(t->kit()))
        return false;

    if (t->project()->projectManager()->mimeType() != QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE))
        return false;

    return true;
}

QList<ProjectExplorer::BuildInfo *> UbuntuHtmlBuildConfigurationFactory::createBuildInfos(const ProjectExplorer::Kit *k, const QString &projectDir) const
{
    QList<ProjectExplorer::BuildInfo *> builds;

    ProjectExplorer::BuildInfo *info = new ProjectExplorer::BuildInfo(this);
    info->buildDirectory = Utils::FileName::fromString(UbuntuProject::shadowBuildDirectory(projectDir,k,QStringLiteral("default")));
    info->typeName = tr("Html5");
    info->kitId    = k->id();
    info->displayName = tr("Default");

    builds << info;
    return builds;
}

UbuntuHtmlBuildSettingsWidget::UbuntuHtmlBuildSettingsWidget(UbuntuHtmlBuildConfiguration *conf, QWidget *parent)
    : NamedWidget(parent) ,
      m_buildConfiguration(conf)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(20, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
    fl->addRow(tr("Build directory:"), m_pathChooser);

    connect(m_pathChooser->lineEdit(),SIGNAL(editingFinished()),this,SLOT(onBuilddirChanged()));
    connect(m_buildConfiguration,SIGNAL(buildDirectoryChanged()),this,SLOT(updateBuildDirectory()));
}

void UbuntuHtmlBuildSettingsWidget::updateBuildDirectory() const
{
    m_pathChooser->blockSignals(true);
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
    m_pathChooser->blockSignals(false);
}


} // namespace Internal
} // namespace Ubuntu
