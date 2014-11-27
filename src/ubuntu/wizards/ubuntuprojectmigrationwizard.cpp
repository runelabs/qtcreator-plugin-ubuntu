#include "ubuntuprojectmigrationwizard.h"

#include "../ubuntuclicktool.h"
#include "../ubuntubzr.h"

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <QCheckBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>

namespace Ubuntu {
namespace Internal {

UbuntuProjectMigrationWizard::UbuntuProjectMigrationWizard(QmakeProjectManager::QmakeProject *project, QWidget *parent) :
    Utils::Wizard(parent),
    m_project(project)
{
    const int selectTargetPageId = addPage(new UbuntuSelectSubProjectsPage);
    const int projectsInfoPageId = addPage(new UbuntuProjectDetailsPage);

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(selectTargetPageId)->setTitle(tr("Targets"));
    progress->item(projectsInfoPageId)->setTitle(tr("Details"));

    setMinimumSize(800,600);
}

QmakeProjectManager::QmakeProject *UbuntuProjectMigrationWizard::project() const
{
    return m_project;
}

QStringList UbuntuProjectMigrationWizard::relevantTargets() const
{
    QStringList result;
    QList<QmakeProjectManager::QmakeProFileNode *> nodes = m_project->allProFiles();
    foreach(const QmakeProjectManager::QmakeProFileNode * node, nodes) {
        if(node->projectType() == QmakeProjectManager::ApplicationTemplate)
            result += node->targetInformation().target;
    }

    return result;
}

UbuntuSelectSubProjectsPage::UbuntuSelectSubProjectsPage(QWidget *parent) : QWizardPage(parent)
{
    QLabel *label = new QLabel(tr("<h2 style=\"text-align: center;\">Please select the applications you want to add to the manifest file</h2>"));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

void UbuntuSelectSubProjectsPage::initializePage()
{
    QWizardPage::initializePage();
    UbuntuProjectMigrationWizard *wiz = qobject_cast<UbuntuProjectMigrationWizard *>(wizard());
    if(wiz) {
        foreach(const QString &targetName, wiz->relevantTargets()) {
            QCheckBox *check = new QCheckBox(targetName);
            check->setChecked(true);
            layout()->addWidget(check);
            connect(check,SIGNAL(stateChanged(int)),this,SIGNAL(completeChanged()));

            registerField(QStringLiteral("addTarget_")+targetName,check);
        }
    }
}

bool UbuntuSelectSubProjectsPage::isComplete() const
{
    //check if at least one item is selected
    UbuntuProjectMigrationWizard *wiz = qobject_cast<UbuntuProjectMigrationWizard *>(wizard());
    if(wiz) {
        foreach(const QString &targetName, wiz->relevantTargets()) {
            if(field(QStringLiteral("addTarget_")+targetName).toBool())
                return true;
        }
    }
    return false;
}

UbuntuProjectDetailsPage::UbuntuProjectDetailsPage(QWidget *parent) :
    QWizardPage(parent),
    m_initialized(false)
{

    QFormLayout *layout = new QFormLayout;

    QLineEdit *domainLineEdit = new QLineEdit;
    layout->addRow(tr("Domain"),domainLineEdit);
    registerField(QStringLiteral("domain*"),domainLineEdit);

    QLineEdit *maintainerLineEdit = new QLineEdit;
    layout->addRow(tr("Maintainer"),maintainerLineEdit);
    registerField(QStringLiteral("maintainer*"),maintainerLineEdit);

    QComboBox *box = new QComboBox;
    QStringList allFrameworks = UbuntuClickFrameworkProvider::getSupportedFrameworks();

    int running    = -1;
    int defaultIdx = -1;
    foreach(const QString &fw, allFrameworks) {
        if(defaultIdx == -1) {
            running++;
            if(!fw.contains(QStringLiteral("-dev")))
                defaultIdx = running;
        }

        box->addItem(fw,fw);
    }

    if(defaultIdx >= 0)
        box->setCurrentIndex(defaultIdx);

    layout->addRow(tr("Framework"),box);
    registerField(QStringLiteral("framework"),box,"currentText",SIGNAL(currentTextChanged(QString)));
    connect(box,SIGNAL(currentTextChanged(QString)),this,SIGNAL(completeChanged()));

    setLayout(layout);
}

void UbuntuProjectDetailsPage::initializePage()
{
    QWizardPage::initializePage();

    if(!m_initialized) {
        m_initialized = true;

        QString maintainer = QStringLiteral("username");
        QString whoami     = QStringLiteral("maintainerName");
        UbuntuBzr *bzr = UbuntuBzr::instance();

        if(!bzr->isInitialized()) {
            bzr->initialize();
            bzr->waitForFinished();
        }

        if(bzr->isInitialized()) {
            maintainer = bzr->launchpadId();
            whoami     = bzr->whoami();
        }

        wizard()->setField(QStringLiteral("domain"),QString(QStringLiteral("com.ubuntu.developer.")+maintainer));
        wizard()->setField(QStringLiteral("maintainer"),whoami);
    }

}

bool UbuntuProjectDetailsPage::isComplete() const
{
    return QWizardPage::isComplete();
}

} // namespace Internal
} // namespace Ubuntu
