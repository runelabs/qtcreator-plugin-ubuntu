#include "ubuntufirstrunwizard.h"
#include "../ubuntuconstants.h"
#include "../ubuntukitmanager.h"
#include "../ubuntuclickdialog.h"
#include "../clicktoolchain.h"
#include "../ubuntudevicesmodel.h"
#include "../ubuntuprocess.h"
#include "../ubuntudevice.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QDebug>
#include <QVariant>
#include <QHeaderView>
#include <QSpacerItem>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuFirstRunWizard::UbuntuFirstRunWizard(QWidget *parent) :
    Utils::Wizard(parent)
{
    const int introPageId    = addPage(new UbuntuIntroductionWizardPage);
    const int chrootsPageId  = addPage(new UbuntuSetupChrootWizardPage);
    const int emulatorPageId = addPage(new UbuntuSetupEmulatorWizardPage);

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(introPageId)->setTitle(tr("Intro"));
    progress->item(chrootsPageId)->setTitle(tr("Kits and Toolchains"));
    progress->item(emulatorPageId)->setTitle(tr("Devices and Emulators"));

    setMinimumSize(800,600);
}



UbuntuIntroductionWizardPage::UbuntuIntroductionWizardPage(QWidget *parent) :
    QWizardPage(parent)
{
    QLabel *label = new QLabel(tr("<h1 style=\"text-align: center;\">Welcome to the Ubuntu-SDK</h1>"
                               "<p>This Wizard will help to setup a development environment to create Applications for the Ubuntu platform.</p>"
                               "<p>At any time later it is possible to create:"
                               "<ul><li>new targets in the  &quot;Tools -&gt; Options -&gt; Ubuntu&quot; Settings Page,</li>"
                               "<li>new emulators on the Devices pages by clicking on the &quot;+&quot; button</li></ul>"
                               "</p>"
                               "<p>Have a lot of fun!</p>"));
    label->setWordWrap(true);

    QCheckBox *check = new QCheckBox(tr("Do not show this Wizard the next time."));
    check->setChecked(true);


    registerField(QStringLiteral("disableWizard"),check);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(check);
    setLayout(layout);
}

void UbuntuIntroductionWizardPage::initializePage()
{

}

bool UbuntuIntroductionWizardPage::isComplete() const
{
    return true;
}

UbuntuSetupChrootWizardPage::UbuntuSetupChrootWizardPage(QWidget *parent)
    : QWizardPage(parent) ,
      m_complete(false)
{

    setTitle(tr("Build targets"));

    QLabel *label = new QLabel(tr("<p>In order to create Apps for the Ubuntu platform, it is required to create Kits. Kits enable cross-platform and cross-configuration development. Kits consist of a set of values that define one environment, such as a target device, sysroot to build against,  toolchain to build with, platform specific API set, and some metadata.</p>"
                                  "<p><strong>Note: </strong>It is recommended to create Kits for each possible target architecture (i386, armhf). When developing with the emulator, the best experience is provided by using a i386 emulator and Kit</p>"));
    label->setWordWrap(true);

    m_kitExistsLabel = new QLabel(tr("These Kits are already available on the machine, but it is also possible to create new ones."));
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
        const Core::Id devId = ProjectExplorer::DeviceKitInformation::deviceId(curr);

        if (tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID) ||
                devId == ProjectExplorer::Constants::DESKTOP_DEVICE_ID) {
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
    UbuntuClickDialog::createClickChrootModal(true,QString(), QString(),this);
    initializePage();
}

UbuntuSetupEmulatorWizardPage::UbuntuSetupEmulatorWizardPage(QWidget *parent)
    : QWizardPage(parent)
{

    QLabel *label = new QLabel(tr("<p>To be able to run applications either a physical device or a Ubuntu Emulator can be used.<br />"
                                  "Please enable the checkbox below if a emulator should be created right away and after pressing the &quot;Finish&quot; button the device page will be opened.</p>"
                                  "<p>Skip that step if only physical devices are used.</p>"
                                  "<p><strong>Note: </strong>Currently we suggest to use only the i386 emulator, because it provides the best experience</p>"));
    label->setWordWrap(true);

    m_createEmulatorCheckBox = new QCheckBox(tr("Create emulator"));
    registerField(QStringLiteral("createEmulator"),m_createEmulatorCheckBox);

    m_devicesList =new QTreeWidget;
    m_devicesList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_devicesList->setSelectionMode(QAbstractItemView::NoSelection);
    m_devicesList->setItemsExpandable(false);
    m_devicesList->header()->setStretchLastSection(true);
    m_devicesList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_devicesList->setColumnCount(1);

    QStringList headers;
    headers << tr("Device Name");
    m_devicesList->setHeaderLabels(headers);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addSpacing(10);
    layout->addWidget(new QLabel(tr("List of available devices")));
    layout->addWidget(m_devicesList);
    layout->addWidget(m_createEmulatorCheckBox);
    setLayout(layout);

    connect(ProjectExplorer::DeviceManager::instance(),SIGNAL(deviceAdded(Core::Id)),this,SLOT(updateDevicesList()));
    connect(ProjectExplorer::DeviceManager::instance(),SIGNAL(deviceRemoved(Core::Id)),this,SLOT(updateDevicesList()));
    connect(ProjectExplorer::DeviceManager::instance(),SIGNAL(deviceListReplaced()),this,SLOT(updateDevicesList()));
}

void UbuntuSetupEmulatorWizardPage::updateDevicesList()
{
    m_devicesList->clear();
    ProjectExplorer::DeviceManager *devMgr = ProjectExplorer::DeviceManager::instance();
    for(int i = 0; i < devMgr->deviceCount(); i++) {
        ProjectExplorer::IDevice::ConstPtr dev = devMgr->deviceAt(i);
        if(!dev)
            continue;

        if(!dev->type().toString().startsWith(QLatin1String(Constants::UBUNTU_DEVICE_TYPE_ID)))
            continue;

        QTreeWidgetItem* devItem = new QTreeWidgetItem;
        devItem->setText(0,dev->displayName());
        m_devicesList->addTopLevelItem(devItem);
    }
}

void UbuntuSetupEmulatorWizardPage::initializePage()
{
    updateDevicesList();
}

bool UbuntuSetupEmulatorWizardPage::isComplete() const
{
    return true;
}

} // namespace Internal
} // namespace Ubuntu
