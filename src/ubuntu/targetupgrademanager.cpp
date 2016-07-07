/*
 * Copyright 2015 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#include "targetupgrademanager.h"

#include "ubuntuconstants.h"
#include "ubuntuclickdialog.h"
#include "ui_targetupgrademanagerdialog.h"
#include "settings.h"

#include <coreplugin/coreplugin.h>
#include <utils/qtcassert.h>

#include <QProcess>
#include <QString>
#include <QPointer>
#include <QProgressDialog>


namespace Ubuntu {
namespace Internal {

TargetUpgradeManager::TargetUpgradeManager(QObject *parent) :
    QObject(parent), m_state(Idle)
{
    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(coreAboutToClose()));
}

void TargetUpgradeManager::checkForUpgrades()
{
    bool set = Settings::chrootSettings().autoCheckForUpdates;
    if(set && m_state == Idle) {
        m_state = CollectPendingUpdates;
        m_outdatedChroots.clear();
        foreach(const UbuntuClickTool::Target &buildTarget, UbuntuClickTool::listAvailableTargets()) {
            if (!buildTarget.upgradesEnabled)
                continue;

            QPointer<QProcess> proc(new QProcess(this));
            connect(proc.data(),SIGNAL(finished(int)),this,SLOT(processFinished()));

            proc->start(QString::fromLatin1(Constants::CHROOT_UPDATE_LIST_SCRIPT)
                        .arg(Constants::UBUNTU_RESOURCE_PATH)
                        .arg(buildTarget.containerName));

            Task t;
            t.proc = proc;
            t.target = buildTarget;
            m_running.insert(reinterpret_cast<qintptr>(proc.data()),t);
        }
    }
}

void TargetUpgradeManager::processFinished()
{
    qintptr id = reinterpret_cast<qintptr>(sender());
    QTC_ASSERT(m_running.contains(id),return);

    switch(m_state) {
        case CollectPendingUpdates:{
            Task task = m_running.take(id);
            task.proc->deleteLater();

            if(task.proc->exitStatus() == QProcess::NormalExit && task.proc->exitCode() > 0)
                m_outdatedChroots.append(task.target);

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

void TargetUpgradeManager::coreAboutToClose()
{
    QProgressDialog dlg;
    dlg.setCancelButton(0);
    dlg.setRange(0,0);
    dlg.setLabelText(tr("Waiting for background processes to terminate."));
    dlg.open();

    for(auto i = m_running.begin(); i != m_running.end(); i++) {
        Task &t = i.value();
        if (t.proc) {
            t.proc->disconnect(this);
            t.proc->terminate();

            //polling is ugly, but in this case there is no clean way of handling this
            //since we need to block in this function until all things are sorted out
            int msecs = 30000;
            int timeframe = 10;
            while(!t.proc->waitForFinished(timeframe)) {

                msecs -= timeframe;
                if (msecs <= 0) {
                    t.proc->kill();
                    break;
                }

                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                if (t.proc->error() != QProcess::Timedout)
                    break;
            }

            delete t.proc;
        }
    }

    m_state = Idle;
    m_running.clear();
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
        QList<UbuntuClickTool::Target> selectedTargets;
        for(int i = 0; i < targets.size(); i++) {
            if(dlg.m_ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked) {
                selectedTargets << targets.at(i);
            }
        }
        if(selectedTargets.size() > 0)
            UbuntuClickDialog::maintainClickModal(selectedTargets,UbuntuClickTool::Upgrade);
    }
}

} // namespace Internal
} // namespace Ubuntu
