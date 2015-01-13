#include "targetupgrademanager.h"

#include "ubuntuconstants.h"
#include "ubuntuclickdialog.h"
#include "ui_targetupgrademanagerdialog.h"

#include <coreplugin/coreplugin.h>

#include <QProcess>
#include <QString>
#include <QPointer>

namespace Ubuntu {
namespace Internal {

const char CHROOT_UPDATE_COMMAND[] = "click chroot -a %1 -f %2 -n %3 maint env LC_ALL=C apt-get update";
const char CHROOT_UPDATE_LIST_COMMAND[] = "click chroot -a %1 -f %2 -n %3 maint env LC_ALL=C apt list --upgradable";

TargetUpgradeManager::TargetUpgradeManager(QObject *parent) :
    QObject(parent), m_state(Idle)
{
}

void TargetUpgradeManager::checkForUpgrades()
{
    if(m_state == Idle) {
        m_state = AptGetUpdate;
        m_outdatedChroots.clear();
        foreach(const UbuntuClickTool::Target &chroot, UbuntuClickTool::listAvailableTargets()) {
            QPointer<QProcess> proc(new QProcess(this));
            connect(proc.data(),SIGNAL(finished(int)),this,SLOT(processFinished()));

            proc->start(QString::fromLatin1(CHROOT_UPDATE_COMMAND)
                        .arg(chroot.architecture)
                        .arg(chroot.framework)
                        .arg(UbuntuClickTool::clickChrootSuffix()));

            Task t;
            t.proc = proc;
            t.target = chroot;
            m_running.insert(reinterpret_cast<qintptr>(proc.data()),t);
        }
    }
}

void TargetUpgradeManager::processFinished()
{
    qintptr id = reinterpret_cast<qintptr>(sender());
    switch(m_state) {
        case AptGetUpdate:{
            m_running.take(id).proc->deleteLater();

            //all tasks done
            if(m_running.isEmpty()) {
                m_state = CollectPendingUpdates;

                foreach(const UbuntuClickTool::Target &chroot, UbuntuClickTool::listAvailableTargets()) {
                    QPointer<QProcess> proc(new QProcess(this));
                    connect(proc.data(),SIGNAL(finished(int)),this,SLOT(processFinished()));

                    proc->start(QString::fromLatin1(CHROOT_UPDATE_LIST_COMMAND)
                                .arg(chroot.architecture)
                                .arg(chroot.framework)
                                .arg(UbuntuClickTool::clickChrootSuffix()));

                    Task t;
                    t.proc = proc;
                    t.target = chroot;
                    m_running.insert(reinterpret_cast<qintptr>(proc.data()),t);
                }

            }

            break;
        }
        case CollectPendingUpdates:{
            Task task = m_running.take(id);
            task.proc->deleteLater();

            if(task.proc->exitCode() == 0) {
                QString stdout = QString::fromLatin1(task.proc->readAllStandardOutput());
                QList<QString> lines = stdout.split(QStringLiteral("\n"),QString::SkipEmptyParts);
                lines.removeFirst();

                if(lines.size() > 0)
                    m_outdatedChroots.append(task.target);
            }

            if(m_running.isEmpty()) {
                m_state = Idle;

                if(m_outdatedChroots.isEmpty())
                    break;

                TargetUpgradeManagerDialog::selectAndUpgradeTargets(m_outdatedChroots,Core::ICore::mainWindow());
                m_outdatedChroots.clear();
            }

            break;
        }
        default:
            break;
    }
}

TargetUpgradeManagerDialog::TargetUpgradeManagerDialog(QWidget *parent) : QDialog(parent)
{
    m_ui = new Ubuntu::Internal::Ui::TargetUpgradeManagerDialog;
    m_ui->setupUi(this);
}

TargetUpgradeManagerDialog::~TargetUpgradeManagerDialog()
{
    delete m_ui;
}

void TargetUpgradeManagerDialog::selectAndUpgradeTargets(QList<UbuntuClickTool::Target> targets,QWidget *parent)
{

    TargetUpgradeManagerDialog dlg(parent);
    for(int i = 0; i < targets.size(); i++) {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setCheckState(0,Qt::Unchecked);
        item->setText(0,targets.at(i).framework+QStringLiteral("-")+targets.at(i).architecture);
        dlg.m_ui->treeWidget->addTopLevelItem(item);
    }

    if( dlg.exec() == QDialog::Accepted ) {
        for(int i = 0; i < targets.size(); i++) {
            if(dlg.m_ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked) {
                UbuntuClickDialog::maintainClickModal(targets.at(i),UbuntuClickTool::Upgrade);
            }
        }
    }
}

} // namespace Internal
} // namespace Ubuntu
