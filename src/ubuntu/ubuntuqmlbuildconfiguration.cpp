#include "ubuntuqmlbuildconfiguration.h"
#include "ubuntuconstants.h"
#include "ubuntuproject.h"

#include <coreplugin/mimedatabase.h>

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/buildsteplist.h>
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
 * targets as well as to build the translations for click packaging
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

    //is this a ubuntu project?
    Utils::FileName manifestFilePath = Utils::FileName::fromString(parent->project()->projectDirectory()).appendPath(QStringLiteral("manifest.json"));
    Utils::FileName makeFilePath = Utils::FileName::fromString(parent->project()->projectDirectory()).appendPath(QStringLiteral("Makefile"));
    if(manifestFilePath.toFileInfo().exists()
            && makeFilePath.toFileInfo().exists()) {

        //make sure the "build-translations" target is available in the Makefile
        QFile makefile(makeFilePath.toString());
        if(makefile.open(QIODevice::ReadOnly)) {
            QByteArray data = makefile.readAll();
            if(data.contains("build-translations:")) {
                ProjectExplorer::BuildStepList *bs = conf->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
                bs->insertStep(0, new UbuntuQmlUpdateTranslationTemplateStep(bs));
                bs->insertStep(1, new UbuntuQmlBuildTranslationStep(bs));
            }
        }
    }
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

UbuntuQmlUpdateTranslationTemplateStep::UbuntuQmlUpdateTranslationTemplateStep(ProjectExplorer::BuildStepList *bsl)
    : UbuntuQmlUpdateTranslationTemplateStep(bsl,Constants::UBUNTU_CLICK_QML_UPDATE_TRANSL_MAKESTEP)
{
    setDefaultDisplayName(tr("Update translations template"));
}

UbuntuQmlUpdateTranslationTemplateStep::UbuntuQmlUpdateTranslationTemplateStep(ProjectExplorer::BuildStepList *bsl,Core::Id typeId)
    : AbstractProcessStep(bsl,typeId)
{}

UbuntuQmlUpdateTranslationTemplateStep::UbuntuQmlUpdateTranslationTemplateStep(ProjectExplorer::BuildStepList *bsl, UbuntuQmlUpdateTranslationTemplateStep *bs)
    : AbstractProcessStep(bsl,bs)
{}

bool UbuntuQmlUpdateTranslationTemplateStep::init()
{
    QString projectDir = target()->project()->projectDirectory();

    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();
    if(!bc)
        return false;

    ProjectExplorer::ProcessParameters *param = processParameters();
    param->setWorkingDirectory(projectDir);
    param->setCommand(makeCommand(ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit()), bc->environment()));
    param->setMacroExpander(bc->macroExpander());
    param->setEnvironment(bc->environment());
    return true;
}

QString UbuntuQmlUpdateTranslationTemplateStep::makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const
{
    if (tc)
        return tc->makeCommand(env);
    return QStringLiteral("make");
}

ProjectExplorer::BuildStepConfigWidget *UbuntuQmlUpdateTranslationTemplateStep::createConfigWidget()
{
    return new ProjectExplorer::SimpleBuildStepConfigWidget(this);
}

UbuntuQmlBuildTranslationStep::UbuntuQmlBuildTranslationStep(ProjectExplorer::BuildStepList *bsl)
    : UbuntuQmlUpdateTranslationTemplateStep(bsl,Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_MAKESTEP)
{
    setDefaultDisplayName(tr("Build translations"));
}

UbuntuQmlBuildTranslationStep::UbuntuQmlBuildTranslationStep(ProjectExplorer::BuildStepList *bsl, UbuntuQmlBuildTranslationStep *bs)
    : UbuntuQmlUpdateTranslationTemplateStep(bsl,bs)
{}

bool UbuntuQmlBuildTranslationStep::init()
{
    if(!UbuntuQmlUpdateTranslationTemplateStep::init())
        return false;

    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();
    if(!bc)
        return false;

    m_translationDir = bc->buildDirectory().toString()
            + QDir::separator()
            + QString::fromLatin1(Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_DIR);

    processParameters()->setArguments(QString::fromLatin1("TRANSLATION_ROOT=%1 build-translations").arg(m_translationDir));
    return true;
}

ProjectExplorer::BuildStepConfigWidget *UbuntuQmlBuildTranslationStep::createConfigWidget()
{
    return new ProjectExplorer::SimpleBuildStepConfigWidget(this);
}

void UbuntuQmlBuildTranslationStep::run(QFutureInterface<bool> &fi)
{
    QDir translDir(m_translationDir);
    if(translDir.exists())
        translDir.removeRecursively();

    return UbuntuQmlUpdateTranslationTemplateStep::run(fi);
}

QList<Core::Id> UbuntuQmlBuildStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if(parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return QList<Core::Id>();

    UbuntuKitMatcher m;
    if(!m.matches(parent->target()->kit())
            || parent->target()->project()->id() != "QmlProjectManager.QmlProject")
        return QList<Core::Id>();

    return QList<Core::Id>()<<Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_MAKESTEP<<Constants::UBUNTU_CLICK_QML_UPDATE_TRANSL_MAKESTEP;
}

QString UbuntuQmlBuildStepFactory::displayNameForId(const Core::Id id) const
{
    if(id == Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_MAKESTEP)
        return tr("Build translations");
    else if(id == Constants::UBUNTU_CLICK_QML_UPDATE_TRANSL_MAKESTEP)
        return tr("Update translations template");
    return QString();
}

bool UbuntuQmlBuildStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::BuildStep *UbuntuQmlBuildStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    QTC_ASSERT(canCreate(parent,id),return 0);

    if(id == Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_MAKESTEP)
        return new UbuntuQmlBuildTranslationStep(parent);
    else if(id == Constants::UBUNTU_CLICK_QML_UPDATE_TRANSL_MAKESTEP)
        return new UbuntuQmlUpdateTranslationTemplateStep(parent);

    return nullptr;
}

bool UbuntuQmlBuildStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return availableCreationIds(parent).contains(ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *UbuntuQmlBuildStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent,map),return 0);

    ProjectExplorer::AbstractProcessStep *step = 0;
    Core::Id id = ProjectExplorer::idFromMap(map);

    if(id == Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_MAKESTEP)
        step = new UbuntuQmlBuildTranslationStep(parent);
    else if(id == Constants::UBUNTU_CLICK_QML_UPDATE_TRANSL_MAKESTEP)
        step = new UbuntuQmlUpdateTranslationTemplateStep(parent);

    if(!step)
        return nullptr;

    if(!step->fromMap(map)) {
        delete step;
        return nullptr;
    }
    return step;
}

bool UbuntuQmlBuildStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return availableCreationIds(parent).contains(product->id());
}

ProjectExplorer::BuildStep *UbuntuQmlBuildStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    QTC_ASSERT(canClone(parent,product),return 0);

    const Core::Id id = product->id();
    if(id == Constants::UBUNTU_CLICK_QML_BUILD_TRANSL_MAKESTEP)
        return new UbuntuQmlBuildTranslationStep(parent, static_cast<UbuntuQmlBuildTranslationStep *>(product));
    else if(id == Core::Id(Constants::UBUNTU_CLICK_QML_UPDATE_TRANSL_MAKESTEP))
        return new UbuntuQmlUpdateTranslationTemplateStep(parent, static_cast<UbuntuQmlUpdateTranslationTemplateStep *>(product));

    return 0;
}

} // namespace Internal
} // namespace Ubuntu
