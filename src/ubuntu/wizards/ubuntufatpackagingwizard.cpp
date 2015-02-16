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
#include <QComboBox>
#include <QDebug>

namespace Ubuntu {
namespace Internal {

UbuntuFatPackagingWizard::UbuntuFatPackagingWizard(QmakeProjectManager::QmakeProject *project,  QWidget *parent) :
    Utils::Wizard(parent),
    m_targetPage(0)
{
    const int introPageId    = addPage(new UbuntuFatPackagingIntroPage(project));

    m_targetPage = new UbuntuChooseTargetPage(project);
    const int targetPageId  = addPage(m_targetPage);

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(introPageId)->setTitle(tr("Intro"));
    progress->item(targetPageId)->setTitle(tr("Targets"));

    setMinimumSize(800,600);
}

QList<ProjectExplorer::BuildConfiguration *> UbuntuFatPackagingWizard::selectedTargets()
{
    return m_targetPage->selectedSuspects();
}

UbuntuFatPackagingIntroPage::UbuntuFatPackagingIntroPage(QmakeProjectManager::QmakeProject *project, QWidget *parent)
    : QWizardPage(parent) ,
      m_project(project)
{
    QLabel *label = new QLabel(tr("<h1 style=\"text-align: center;\">Click packaging</h1>"
                               "<p>This Wizard will help to build a click package for uploading into the store.</p>"
                               "<p>If the project has native parts, select the \"Architecture dependent\" mode,<br>otherwise "
                               "\"Architecture independent\" should be selected.<br>Also choose the output directory where the final click "
                               "package will be created."
                               "<br></p>"),this);
    label->setWordWrap(true);

    m_modeBox = new QComboBox(this);
    m_modeBox->addItem(tr("Architecture independent"));
    m_modeBox->addItem(tr("Architecture dependent"));
    registerField(QStringLiteral("mode"),m_modeBox);

    connect(m_modeBox,SIGNAL(currentIndexChanged(int)),this,SLOT(buildModeChanged(int)));

    QString path = QString::fromLatin1("/tmp/click-build-%1").arg(m_project->displayName());

    m_clickPackagePath = new Utils::PathChooser(this);
    m_clickPackagePath->setPath(path);
    m_clickPackagePath->setExpectedKind(Utils::PathChooser::Directory);
    registerField(QStringLiteral("targetDirectory"),m_clickPackagePath,"path","pathChanged");
    connect(m_clickPackagePath,SIGNAL(pathChanged(QString)),this,SIGNAL(completeChanged()));

    QFormLayout *layout = new QFormLayout;
    layout->addRow(label);
    layout->addRow(tr("Mode"),m_modeBox);
    layout->addRow(tr("Destination directory"),m_clickPackagePath);
    setLayout(layout);
}

void UbuntuFatPackagingIntroPage::initializePage()
{
    bool likelyHasBinaryParts = false;
    QList<QmakeProjectManager::QmakeProFileNode *> nodes = m_project->allProFiles();
    foreach(QmakeProjectManager::QmakeProFileNode * node, nodes) {
        if (node->projectType() == QmakeProjectManager::ApplicationTemplate
                || node->projectType() == QmakeProjectManager::LibraryTemplate ) {
            likelyHasBinaryParts = true;
            break;
        }
    }

    if (likelyHasBinaryParts)
        m_modeBox->setCurrentIndex(1);
    else
        m_modeBox->setCurrentIndex(0);
}

bool UbuntuFatPackagingIntroPage::isComplete() const
{
    return m_clickPackagePath->isValid();
}

int UbuntuFatPackagingIntroPage::nextId() const
{
    if (m_modeBox->currentIndex() == 0)
        return -1;

    return QWizardPage::nextId();
}

void UbuntuFatPackagingIntroPage::buildModeChanged(const int mode)
{
    setFinalPage( mode == 0 );
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

    connect(m_targetView, &QTreeWidget::itemChanged, this, &UbuntuChooseTargetPage::completeChanged);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setVisible(false);
    m_errorLabel->setStyleSheet(QStringLiteral("background: yellow"));

    m_noTargetsLabel = new QLabel(tr("<h3 style=\"text-align: center;\">No Targets available</h3>"
                                     "<p>In order to build a click package, Ubuntu Kits need to be "
                                     "added to the project. This is possible by adding them in the "
                                     "\"Projects\" tab.</p>"),this);
    m_noTargetsLabel->setWordWrap(true);
    m_noTargetsLabel->hide();

    QFormLayout *layout = new QFormLayout;
    layout->addRow(new QLabel(tr("Architectures"),this),m_targetView);
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
