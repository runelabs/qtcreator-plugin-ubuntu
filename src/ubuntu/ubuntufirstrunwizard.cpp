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
#include <QTextEdit>
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
    page->setTitle(tr("Introduction"));

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
    QLabel *label = new QLabel(tr("In order to create Apps for the Ubuntu platform, you need to create click chroots that contain the development tools and libraries for the devices."));
    label->setWordWrap(true);

    m_kitExistsLabel = new QLabel(tr("These click chroots are already available on your machine, but you can still create new ones."));
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
    headers << tr("Chroot Target Name");
    m_kitList->setHeaderLabels(headers);


    m_createKitButton = new QPushButton(tr("Create new chroot"));
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

    m_proc = new UbuntuProcess(this);
    connect(m_proc,SIGNAL(message(QString)),this,SLOT(onProcMessage(QString)));
    connect(m_proc,SIGNAL(finished(QString,int)),this,SLOT(onProcFinished(QString,int)));

    QLabel *label = new QLabel(tr("<p>To be able to run your applications you either need a physical device, or a Ubuntu Emulator.<br />"
                                  "Please select the Kits you want to create a emulator for and click the &quot;Create emulators&quot; button.</p>"
                                  "<p>Skip that step if you just want to use your physical device.</p>"
                                  "<p><strong>Note: </strong>Currently we suggest to use only the i386 emulator, because it provides the best expirience</p>"));
    label->setWordWrap(true);

    m_kitList = new QTreeWidget;
    m_kitList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_kitList->setSelectionMode(QAbstractItemView::NoSelection);
    m_kitList->setItemsExpandable(false);
    m_kitList->header()->setStretchLastSection(true);
    m_kitList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_kitList->setColumnCount(2);

    QStringList headers;
    headers << tr("Chroot Target Name")<< tr("Create emulators");
    m_kitList->setHeaderLabels(headers);

    m_procOutput = new QTextEdit;
    m_procOutput->setReadOnly(true);

    m_createButton = new QPushButton(tr("Create emulators"));
    connect(m_createButton,SIGNAL(clicked()),this,SLOT(onCreateEmulatorsClicked()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(m_kitList);
    layout->addWidget(m_procOutput);
    layout->addWidget(m_createButton);
    setLayout(layout);
}

void UbuntuSetupEmulatorWizardPage::initializePage()
{
    m_kitList->clear();

    QList<ProjectExplorer::Kit *> allKits = ProjectExplorer::KitManager::kits();
   // QAbstractItemModel* model = m_kitList->model();

    for(int i = 0; i < allKits.length(); i++) {
        ProjectExplorer::Kit *curr = allKits.at(i);
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(curr);
        if (tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)) {
            if(ProjectExplorer::DeviceKitInformation::deviceId(curr).isValid())
                continue;

            //this kit has no valid Device lets add it to the list
            QTreeWidgetItem* kitItem = new QTreeWidgetItem;
            kitItem->setText(0,curr->displayName());
            kitItem->setData(0,Qt::UserRole,curr->id().uniqueIdentifier());
            m_kitList->addTopLevelItem(kitItem);

            kitItem->setCheckState(1,Qt::Unchecked);
        }
    }
}

bool UbuntuSetupEmulatorWizardPage::isComplete() const
{
    return m_kitsToCreate.size() == 0;
}

void UbuntuSetupEmulatorWizardPage::onCreateEmulatorsClicked()
{
    for (int i = 0; i < m_kitList->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = m_kitList->topLevelItem(i);
        if(item->checkState(1) == Qt::Unchecked)
            continue;

        bool ok = false;
        int kitUId = item->data(0,Qt::UserRole).toInt(&ok);

        if(!ok)
            continue;

        ProjectExplorer::Kit *kit = ProjectExplorer::KitManager::find(Core::Id(kitUId));
        if(!kit)
            continue;

        m_kitsToCreate.append(kit);
    }

    onProcFinished(QString(),0);
}

void UbuntuSetupEmulatorWizardPage::onProcMessage(const QString &text)
{
    QTextCursor cursor(m_procOutput->document());
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat tf;

    QFont font = m_procOutput->font();
    tf.setFont(font);
    tf.setForeground(m_procOutput->palette().color(QPalette::Text));

    cursor.insertText(text, tf);

    cursor.movePosition(QTextCursor::End);
    m_procOutput->setTextCursor(cursor);
}

void UbuntuSetupEmulatorWizardPage::onProcFinished(const QString &program, int code)
{
    Q_UNUSED(program);

    if (code != 0) {
        QTextCursor cursor(m_procOutput->document());
        QTextCharFormat tf;

        QFont font = m_procOutput->font();
        QFont boldFont = font;
        boldFont.setBold(true);
        tf.setFont(boldFont);
        tf.setForeground(QColor(Qt::red));
        cursor.insertText(tr("----- There was a error when executing the emulator creation script -----"), tf);

        cursor.movePosition(QTextCursor::End);
        m_procOutput->setTextCursor(cursor);
    }

    if (m_kitsToCreate.size() > 0) {
        emit completeChanged();

        //noone can click anything
        m_createButton->setEnabled(false);
        wizard()->button(QWizard::BackButton)->setEnabled(false);
        wizard()->button(QWizard::CancelButton)->setEnabled(false);

        ProjectExplorer::Kit *kit = m_kitsToCreate.takeFirst();

        //no need to do a qobject_cast we only added ubuntu kits to our list
        ClickToolChain *tc = static_cast<ClickToolChain*>(ProjectExplorer::ToolChainKitInformation::toolChain(kit));
        const UbuntuClickTool::Target &target = tc->clickTarget();

        UbuntuDevicesModel::doCreateEmulatorImage(m_proc,
                                                  QStringLiteral("emulator-%1-%2").arg(target.framework).arg(target.architecture),
                                                  tc->clickTarget().architecture,
                                                  QStringLiteral("devel-proposed"));
        return;
    }

    emit completeChanged();
    wizard()->button(QWizard::BackButton)->setEnabled(true);
    wizard()->button(QWizard::CancelButton)->setEnabled(true);
    m_createButton->setEnabled(true);
    initializePage();
}

} // namespace Internal
} // namespace Ubuntu
