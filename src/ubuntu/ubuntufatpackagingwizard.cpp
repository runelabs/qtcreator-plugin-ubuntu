#include "ubuntufatpackagingwizard.h"
#include "ubuntuclicktool.h"
#include "ubuntuconstants.h"
#include "clicktoolchain.h"

#include <utils/pathchooser.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTreeWidget>
#include <QDebug>

namespace Ubuntu {
namespace Internal {

UbuntuFatPackagingWizard::UbuntuFatPackagingWizard(QList<ProjectExplorer::BuildConfiguration *> suspects, QWidget *parent) :
    Utils::Wizard(parent)
{
    const int introPageId    = addPage(new UbuntuFatPackagingIntroPage);
    const int targetPageId  = addPage(new UbuntuChooseTargetPage(suspects));

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(introPageId)->setTitle(tr("Intro"));
    progress->item(targetPageId)->setTitle(tr("Kits and Toolchains"));

    setMinimumSize(800,600);
}

UbuntuFatPackagingIntroPage::UbuntuFatPackagingIntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    QLabel *label = new QLabel(tr("<h1 style=\"text-align: center;\">Click packaging</h1>"
                               "<p>This Wizard will help to build a click package.</p>"
                               "<p>On the next page, select the architectures you want to include into the package"
                               " as well as the destination directory for the final click package."
                               "</p>"));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addStretch();
    setLayout(layout);
}

void UbuntuFatPackagingIntroPage::initializePage()
{

}

bool UbuntuFatPackagingIntroPage::isComplete() const
{
    return true;
}

UbuntuChooseTargetPage::UbuntuChooseTargetPage(QList<ProjectExplorer::BuildConfiguration *> suspects, QWidget *parent)
    : QWizardPage(parent),
      m_suspects(suspects)
{
    QString path = QString::fromLatin1("/tmp/click-build-%1").arg(suspects.first()->target()->project()->displayName());

    Utils::PathChooser *clickPackagePath = new Utils::PathChooser(this);
    clickPackagePath->setPath(path);
    clickPackagePath->setExpectedKind(Utils::PathChooser::Directory);

    m_targetView = new QTreeWidget(this);
    QSizePolicy sizePolicy = m_targetView->sizePolicy();
    sizePolicy.setVerticalPolicy(QSizePolicy::Expanding);
    sizePolicy.setVerticalStretch(1);
    m_targetView->setSizePolicy(sizePolicy);

    connect(m_targetView, &QTreeWidget::itemChanged, this, &UbuntuChooseTargetPage::itemChanged);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setVisible(false);
    m_errorLabel->setStyleSheet(QStringLiteral("background: yellow"));

    QFormLayout *layout = new QFormLayout;
    layout->addRow(new QLabel(tr("Destination directory"),this),clickPackagePath);
    layout->addRow(new QLabel(tr("Architectures"),this),m_targetView);
    layout->addRow(m_errorLabel);
    setLayout(layout);

    registerField(QStringLiteral("targetDirectory"),clickPackagePath,"path","pathChanged");
    registerField(QStringLiteral("selectedSuspects"),this,"selectedSuspects","selectedSuspectsChanged");
}

void UbuntuChooseTargetPage::initializePage()
{
    m_targetView->clear();
    foreach(ProjectExplorer::BuildConfiguration *conf, m_suspects) {
        QTreeWidgetItem* targetItem = new QTreeWidgetItem;
        targetItem->setText(0,conf->target()->displayName()+QStringLiteral(" ")+conf->displayName());
        targetItem->setCheckState(0,Qt::Unchecked);
        m_targetView->addTopLevelItem(targetItem);
    }
}

bool UbuntuChooseTargetPage::isComplete() const
{
    if (!m_selectedSuspects.isEmpty())
        return isValid();
    return false;
}

bool UbuntuChooseTargetPage::validatePage()
{
    return isValid();
}

QList<int> UbuntuChooseTargetPage::selectedSuspects() const
{
    return m_selectedSuspects;
}

bool UbuntuChooseTargetPage::isValid() const
{
    m_errorLabel->hide();

    QString fw;
    foreach (int sel, m_selectedSuspects) {
        ProjectExplorer::BuildConfiguration *conf = m_suspects[sel];
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

void UbuntuChooseTargetPage::itemChanged(QTreeWidgetItem *, int)
{
    m_selectedSuspects.clear();
    for(int i = 0; i < m_suspects.size(); i++) {
        if(m_targetView->topLevelItem(i)->checkState(0) == Qt::Checked)
            m_selectedSuspects << i;
    }

    emit selectedSuspectsChanged(m_selectedSuspects);
    emit completeChanged();
}

} // namespace Internal
} // namespace Ubuntu
