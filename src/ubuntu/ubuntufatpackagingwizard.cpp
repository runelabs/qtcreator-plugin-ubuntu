#include "ubuntufatpackagingwizard.h"

#include <utils/pathchooser.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>

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

    Utils::PathChooser *clickPackagePath = new Utils::PathChooser(this);
    clickPackagePath->setPath(QStringLiteral("/tmp/something"));
    clickPackagePath->setExpectedKind(Utils::PathChooser::Directory);

    m_targetView = new QTreeWidget(this);
    connect(m_targetView, &QTreeWidget::itemChanged, this, &UbuntuChooseTargetPage::itemChanged);

    QFormLayout *layout = new QFormLayout;
    layout->addRow(new QLabel(tr("Select the destination directory"),this),clickPackagePath);
    layout->addRow(new QLabel(tr("Select the architectures"),this),m_targetView);
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
        targetItem->setData(0,Qt::UserRole,conf->id().uniqueIdentifier());
        targetItem->setCheckState(0,Qt::Unchecked);
        m_targetView->addTopLevelItem(targetItem);
    }
}

bool UbuntuChooseTargetPage::isComplete() const
{
    return m_selectedSuspects.size();
}

QList<int> UbuntuChooseTargetPage::selectedSuspects() const
{
    return m_selectedSuspects;
}

void UbuntuChooseTargetPage::itemChanged(QTreeWidgetItem *, int)
{
    m_selectedSuspects.clear();
    for(int i = 0; i < m_suspects.size(); i++) {
        if(m_targetView->topLevelItem(i)->checkState(0) == Qt::Checked) {
            qDebug()<<"Adding item "<<i<<"to teh list";
            m_selectedSuspects << i;
        }
    }

    emit selectedSuspectsChanged(m_selectedSuspects);
    emit completeChanged();
}

} // namespace Internal
} // namespace Ubuntu
