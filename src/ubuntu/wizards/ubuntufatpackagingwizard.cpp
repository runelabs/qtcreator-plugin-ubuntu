#include "ubuntufatpackagingwizard.h"
#include "../ubuntuclicktool.h"
#include "../ubuntuconstants.h"
#include "../clicktoolchain.h"

#include <utils/pathchooser.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTreeWidget>
#include <QRadioButton>
#include <QDebug>
#include <QButtonGroup>

namespace Ubuntu {
namespace Internal {

UbuntuFatPackagingWizard::UbuntuFatPackagingWizard(QmakeProjectManager::QmakeProject *project,  QWidget *parent) :
    Utils::Wizard(parent),
    m_targetPage(0)
{
    m_introPage = new UbuntuFatPackagingIntroPage(project);
    const int introPageId = addPage(m_introPage);

    m_targetPage = new UbuntuChooseTargetPage(project);
    const int targetPageId = addPage(m_targetPage);

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(introPageId)->setTitle(tr("Intro"));
    progress->item(targetPageId)->setTitle(tr("Targets"));

    setMinimumSize(800,600);
}

QList<ProjectExplorer::BuildConfiguration *> UbuntuFatPackagingWizard::selectedTargets()
{
    return m_targetPage->selectedSuspects();
}

UbuntuFatPackagingWizard::BuildMode UbuntuFatPackagingWizard::mode() const
{
    return m_introPage->mode();
}

UbuntuFatPackagingIntroPage::UbuntuFatPackagingIntroPage(QmakeProjectManager::QmakeProject *project, QWidget *parent)
    : QWizardPage(parent) ,
      m_project(project)
{
    QLabel *label = new QLabel(tr("<h1><em>Click packaging</em></h1>"
                                  "<p>This Wizard will help to build a click package for uploading into the store.</p>"
                                  "<p>If the project has native compiled parts like executables and libraries, select the<br />"
                                  "&quot;<em>Project with binary executables and libraries</em>&quot; mode,&nbsp;<br />"
                                  "otherwise &quot;<em>Project without binary executables and libraries (pure qml/html projects)</em>&quot;<br />"
                                  "should be selected.&nbsp;<br/></p>"),this);
    label->setWordWrap(true);

    QRadioButton *compiledPartsButton = new QRadioButton(tr("Project with binary executables and libraries."), this);
    QRadioButton *noCompiledPartsButton = new QRadioButton(tr("Project without binary executables and libraries (pure qml/html projects)"), this);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->addButton(compiledPartsButton, UbuntuFatPackagingWizard::CompiledPartsMode);
    m_modeGroup->addButton(noCompiledPartsButton, UbuntuFatPackagingWizard::NoCompiledPartsMode);
    connect(m_modeGroup, SIGNAL(buttonClicked(int)), this, SLOT(buildModeChanged(int)));

    QString path = QString::fromLatin1("/tmp/click-build-%1").arg(m_project->displayName());

    m_clickPackagePath = new Utils::PathChooser(this);
    m_clickPackagePath->setPath(path);
    m_clickPackagePath->setExpectedKind(Utils::PathChooser::Directory);
    registerField(QStringLiteral("targetDirectory"),m_clickPackagePath,"path","pathChanged");
    connect(m_clickPackagePath,SIGNAL(pathChanged(QString)),this,SIGNAL(completeChanged()));

    QFormLayout *layout = new QFormLayout;
    layout->addRow(label);
    layout->addRow(tr("Destination directory"),m_clickPackagePath);
    layout->addRow(compiledPartsButton);
    layout->addRow(noCompiledPartsButton);
    setLayout(layout);
}

UbuntuFatPackagingWizard::BuildMode UbuntuFatPackagingIntroPage::mode() const
{
    return static_cast<UbuntuFatPackagingWizard::BuildMode>(m_modeGroup->checkedId());
}

void UbuntuFatPackagingIntroPage::initializePage()
{
    bool likelyHasBinaryParts = false;
    QList<QmakeProjectManager::QmakeProFileNode *> nodes = m_project->allProFiles();
    foreach(QmakeProjectManager::QmakeProFileNode * node, nodes) {
        if (node->projectType() == QmakeProjectManager::ApplicationTemplate
                || node->projectType() == QmakeProjectManager::SharedLibraryTemplate
                || node->projectType() == QmakeProjectManager::StaticLibraryTemplate) {
            likelyHasBinaryParts = true;
            break;
        }
    }

    if (likelyHasBinaryParts)
        m_modeGroup->button(UbuntuFatPackagingWizard::CompiledPartsMode)->setChecked(true);
    else
        m_modeGroup->button(UbuntuFatPackagingWizard::NoCompiledPartsMode)->setChecked(true);
}

bool UbuntuFatPackagingIntroPage::isComplete() const
{
    return m_clickPackagePath->isValid();
}

int UbuntuFatPackagingIntroPage::nextId() const
{
    if (m_modeGroup->checkedId() == UbuntuFatPackagingWizard::NoCompiledPartsMode)
        return -1;

    return QWizardPage::nextId();
}

void UbuntuFatPackagingIntroPage::buildModeChanged(const int mode)
{
    setFinalPage( mode == UbuntuFatPackagingWizard::NoCompiledPartsMode );
}

UbuntuChooseTargetPage::UbuntuChooseTargetPage(QmakeProjectManager::QmakeProject *project, QWidget *parent)
    : QWizardPage(parent),
      m_project(project)
{
    m_targetView = new QTreeWidget(this);
    QSizePolicy sizePolicy = m_targetView->sizePolicy();
    sizePolicy.setVerticalPolicy(QSizePolicy::Expanding);
    sizePolicy.setVerticalStretch(1);
    m_targetView->setSizePolicy(sizePolicy);

    m_targetView->setHeaderLabel(tr("Architectures"));

    connect(m_targetView, &QTreeWidget::itemChanged, this, &UbuntuChooseTargetPage::completeChanged);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setVisible(false);
    m_errorLabel->setStyleSheet(QStringLiteral("background: yellow"));

    m_noTargetsLabel = new QLabel(tr("<h3 style=\"text-align: center;\">No Targets available</h3>"
                                     "<p>In order to build a click package, Ubuntu Device Kits need to be "
                                     "added to the project. This is possible by adding them in the "
                                     "\"Projects\" tab.</p>"),this);
    m_noTargetsLabel->setWordWrap(true);
    m_noTargetsLabel->hide();

    QFormLayout *layout = new QFormLayout;
    layout->addRow(m_targetView);
    layout->addRow(m_noTargetsLabel);
    layout->addRow(m_errorLabel);
    setLayout(layout);
}

void UbuntuChooseTargetPage::initializePage()
{
    m_suspects.clear();
    m_targetView->clear();

    foreach(ProjectExplorer::Target *t , m_project->targets()) {

        ProjectExplorer::Kit* k = t->kit();
        if(!k)
            continue;

        if(!ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k).toString().startsWith(QLatin1String(Ubuntu::Constants::UBUNTU_DEVICE_TYPE_ID)))
            continue;

        foreach(ProjectExplorer::BuildConfiguration *b, t->buildConfigurations()) {
            if(b->buildType() == ProjectExplorer::BuildConfiguration::Debug)
                continue;
            m_suspects << b;
        }
    }

    if (m_suspects.isEmpty()) {
        //show some label that tells the user to add Kits
        m_targetView->hide();
        m_noTargetsLabel->show();
    } else {
        m_targetView->show();
        m_noTargetsLabel->hide();

        foreach(ProjectExplorer::BuildConfiguration *conf, m_suspects) {
            QTreeWidgetItem* targetItem = new QTreeWidgetItem;
            targetItem->setText(0,conf->target()->displayName()+QStringLiteral(" ")+conf->displayName());
            targetItem->setCheckState(0,Qt::Unchecked);
            m_targetView->addTopLevelItem(targetItem);
        }
    }
}

bool UbuntuChooseTargetPage::isComplete() const
{
    if (!selectedSuspects().isEmpty())
        return isValid();
    return false;
}

bool UbuntuChooseTargetPage::validatePage()
{
    return isValid();
}

QList<ProjectExplorer::BuildConfiguration *> UbuntuChooseTargetPage::selectedSuspects() const
{
    QList<ProjectExplorer::BuildConfiguration *> selected;
    for(int i = 0; i < m_suspects.size(); i++) {
        if(m_targetView->topLevelItem(i)->checkState(0) == Qt::Checked)
            selected << m_suspects[i];
    }

    return selected;
}

bool UbuntuChooseTargetPage::isValid() const
{
    m_errorLabel->hide();

    QString fw;
    foreach (ProjectExplorer::BuildConfiguration *conf, selectedSuspects()) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(conf->target()->kit());
        if (tc && tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
            ClickToolChain *cTc = static_cast<ClickToolChain *>(tc);
            if (fw.isEmpty())
                fw = cTc->clickTarget().framework;
            else if (fw != cTc->clickTarget().framework) {
                m_errorLabel->setText(tr("It is not possible to combine different frameworks in the same click package!"));
                m_errorLabel->show();
                return false;
            }
        }
    }

    return true;
}

} // namespace Internal
} // namespace Ubuntu
