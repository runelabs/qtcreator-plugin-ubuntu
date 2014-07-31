#include "ubuntufirstrunwizard.h"
#include "ubuntuconstants.h"
#include "ubuntukitmanager.h"
#include "ubuntuclickdialog.h"
#include "clicktoolchain.h"
#include "ubuntudevicesmodel.h"
#include "ubuntuprocess.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QDebug>
#include <QVariant>
#include <QHeaderView>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 1
};

UbuntuFirstRunWizard::UbuntuFirstRunWizard(QWidget *parent) :
    QWizard(parent)
{
    QWizardPage *page = new QWizardPage;
    QLabel *label = new QLabel(tr("<h1 style=\"text-align: center;\">Welcome to the Ubuntu-SDK</h1>"
                               "<p>This Wizard will help you to setup your development environment and enables you to create Applications for the Ubuntu platform.</p>"
                               "<p>The next time you start QtCreator, this Wizard will not be shown anymore, but you can always create new Targets in the Tools -&gt; Options -&gt; Ubuntu Settings Page, "
                               "or using the Devicepage and select &quot;Autocreate kit&quot; when a device is attached.</p>"
                               "<p>Have a lot of fun!</p>"));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    page->setLayout(layout);

    addPage(page);
    addPage(new UbuntuSetupChrootWizardPage);
    addPage(new UbuntuSetupEmulatorWizardPage);

    setMinimumSize(800,600);
}

UbuntuSetupChrootWizardPage::UbuntuSetupChrootWizardPage(QWidget *parent)
    : QWizardPage(parent) ,
      m_complete(false)
{

    setTitle(tr("Build targets"));

    QLabel *label = new QLabel(tr("<p>In order to create Apps for the Ubuntu platform, you need to create Kits that contain the development tools and libraries for the devices.</p>"
                                  "<p><strong>Note: </strong>If you want to develop using the Emulator, we suggest to use a i386 Kit and Emulator to get the best experience</p>"));
    label->setWordWrap(true);

    m_kitExistsLabel = new QLabel(tr("These Kits are already available on your machine, but you can still create new ones."));
    m_kitExistsLabel->setWordWrap(true);
    m_kitExistsLabel->setVisible(false);

    m_kitList = new QTreeWidget;
    m_kitList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_kitList->setSelectionMode(QAbstractItemView::NoSelection);
    m_kitList->setItemsExpandable(false);
    m_kitList->header()->setStretchLastSection(true);
    m_kitList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_kitList->setColumnCount(1);

    QStringList headers;
    headers << tr("Kit Name");
    m_kitList->setHeaderLabels(headers);


    m_createKitButton = new QPushButton(tr("Create new Kit"));
    connect(m_createKitButton,SIGNAL(clicked()),this,SLOT(onCreateKitButtonClicked()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(m_kitExistsLabel);
    layout->addWidget(m_kitList);
    layout->addWidget(m_createKitButton);
    setLayout(layout);
}

void UbuntuSetupChrootWizardPage::initializePage()
{
    QList<ProjectExplorer::Kit *> allKits = ProjectExplorer::KitManager::kits();
    m_kitList->clear();

    bool found = false;
    foreach(ProjectExplorer::Kit *curr, allKits) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(curr);
        if (tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
            found = true;

            QTreeWidgetItem* kitItem = new QTreeWidgetItem;
            kitItem->setText(0,curr->displayName());
            kitItem->setData(0,Qt::UserRole,curr->id().uniqueIdentifier());
            m_kitList->addTopLevelItem(kitItem);
        }
    }

    m_kitExistsLabel->setVisible(found);
    if(m_complete != found) {
        m_complete = found;
        emit completeChanged();
    }
}

bool UbuntuSetupChrootWizardPage::isComplete() const
{
    return m_complete;
}

void UbuntuSetupChrootWizardPage::onCreateKitButtonClicked()
{
    UbuntuClickDialog::createClickChrootModal();
    initializePage();
}

UbuntuSetupEmulatorWizardPage::UbuntuSetupEmulatorWizardPage(QWidget *parent)
    : QWizardPage(parent)
{

    QLabel *label = new QLabel(tr("<p>To be able to run your applications you either need a physical device, or a Ubuntu Emulator.<br />"
                                  "Please enable the checkbox below if you want to create a emulator right away, after pressing the &quot;Finish&quot; button the device page will be opened.</p>"
                                  "<p>Skip that step if you just want to use your physical device.</p>"
                                  "<p><strong>Note: </strong>Currently we suggest to use only the i386 emulator, because it provides the best experience</p>"));
    label->setWordWrap(true);

    m_createEmulatorCheckBox = new QCheckBox(tr("Create emulator"));
    registerField(QStringLiteral("createEmulator"),m_createEmulatorCheckBox);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(m_createEmulatorCheckBox);
    setLayout(layout);
}

void UbuntuSetupEmulatorWizardPage::initializePage()
{

}

bool UbuntuSetupEmulatorWizardPage::isComplete() const
{
    return true;
}

} // namespace Internal
} // namespace Ubuntu
