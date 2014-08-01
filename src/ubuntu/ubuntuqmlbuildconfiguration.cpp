#include "ubuntuqmlbuildconfiguration.h"
#include "ubuntuconstants.h"
#include "ubuntuproject.h"

#include <coreplugin/mimedatabase.h>

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <utils/fancylineedit.h>
#include <qmlprojectmanager/qmlprojectconstants.h>

#include <QFormLayout>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuQmlBuildConfiguration
 *
 * Even though QML projects don't need to be built, we need
 * that BuildConfiguration to store the Builddirectory (where the click
 * package goes) and to enable the Target page to show us some actual
 * targets
 */

UbuntuQmlBuildConfiguration::UbuntuQmlBuildConfiguration(ProjectExplorer::Target *target)
    : BuildConfiguration(target,Constants::UBUNTU_CLICK_QML_BC_ID)
{
}

UbuntuQmlBuildConfiguration::UbuntuQmlBuildConfiguration(ProjectExplorer::Target *target, UbuntuQmlBuildConfiguration *source)
    : BuildConfiguration(target,source)
{

}

bool UbuntuQmlBuildConfiguration::fromMap(const QVariantMap &map)
{
    return BuildConfiguration::fromMap(map);
}

QVariantMap UbuntuQmlBuildConfiguration::toMap() const
{
    return BuildConfiguration::toMap();
}

ProjectExplorer::NamedWidget *UbuntuQmlBuildConfiguration::createConfigWidget()
{
    return new UbuntuQmlBuildSettingsWidget(this);
}

ProjectExplorer::BuildConfiguration::BuildType UbuntuQmlBuildConfiguration::buildType() const
{
    return Release;
}

UbuntuQmlBuildConfigurationFactory::UbuntuQmlBuildConfigurationFactory(QObject *parent)
    : IBuildConfigurationFactory(parent)
{

}

UbuntuQmlBuildConfigurationFactory::~UbuntuQmlBuildConfigurationFactory()
{

}

int UbuntuQmlBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    if (canHandle(parent))
        return 100;
    return -1;
}

QList<ProjectExplorer::BuildInfo *> UbuntuQmlBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target *parent) const
{
    if(!canHandle(parent))
        return QList<ProjectExplorer::BuildInfo *>();
    return createBuildInfos(parent->kit(),parent->project()->projectFilePath());
}

int UbuntuQmlBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    return (k && Core::MimeDatabase::findByFile(QFileInfo(projectPath))
            .matchesType(QLatin1String(QmlProjectManager::Constants::QMLPROJECT_MIMETYPE))) ? 100 : -1;
}

QList<ProjectExplorer::BuildInfo *> UbuntuQmlBuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    UbuntuKitMatcher m;
    if(priority(k,projectPath) < 0 || !m.matches(k))
        return QList<ProjectExplorer::BuildInfo *>();

    return createBuildInfos(k,projectPath);
}

UbuntuQmlBuildConfiguration *UbuntuQmlBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    UbuntuQmlBuildConfiguration *conf = new UbuntuQmlBuildConfiguration(parent);
    conf->setBuildDirectory(info->buildDirectory);
    conf->setDefaultDisplayName(info->displayName);
    conf->setDisplayName(info->displayName);

    return conf;
}

bool UbuntuQmlBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == Constants::UBUNTU_CLICK_QML_BC_ID;
}

UbuntuQmlBuildConfiguration *UbuntuQmlBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent,map) )
        return 0;

    UbuntuQmlBuildConfiguration *conf = new UbuntuQmlBuildConfiguration(parent);
    if (conf->fromMap(map))
        return conf;

    delete conf;
    return 0;
}

bool UbuntuQmlBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const
{
    if (!canHandle(parent))
        return false;
    if (product->id() != Constants::UBUNTU_CLICK_QML_BC_ID )
        return false;

    return true;
}

UbuntuQmlBuildConfiguration *UbuntuQmlBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product)
{
    if (!canClone(parent,product))
        return 0;
    return new UbuntuQmlBuildConfiguration(parent,static_cast<UbuntuQmlBuildConfiguration*>(product));
}

bool UbuntuQmlBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    UbuntuKitMatcher m;
    if(!m.matches(t->kit()))
        return false;

    if (t->project()->id() != "QmlProjectManager.QmlProject")
        return false;

    return true;
}

QList<ProjectExplorer::BuildInfo *> UbuntuQmlBuildConfigurationFactory::createBuildInfos(const ProjectExplorer::Kit *k, const QString &projectDir) const
{
    QList<ProjectExplorer::BuildInfo *> builds;

    ProjectExplorer::BuildInfo *info = new ProjectExplorer::BuildInfo(this);
    info->buildDirectory = Utils::FileName::fromString(UbuntuProject::shadowBuildDirectory(projectDir,k,QStringLiteral("default")));
    info->typeName = tr("Qml");
    info->kitId    = k->id();
    info->supportsShadowBuild = true;
    info->displayName = tr("Default");

    builds << info;
    return builds;
}

UbuntuQmlBuildSettingsWidget::UbuntuQmlBuildSettingsWidget(UbuntuQmlBuildConfiguration *conf, QWidget *parent)
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

void UbuntuQmlBuildSettingsWidget::updateBuildDirectory() const
{
    m_pathChooser->blockSignals(true);
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
    m_pathChooser->blockSignals(false);
}


} // namespace Internal
} // namespace Ubuntu
