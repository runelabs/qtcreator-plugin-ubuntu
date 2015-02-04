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

#ifndef UBUNTU_INTERNAL_TARGETUPGRADEMANAGER_H
#define UBUNTU_INTERNAL_TARGETUPGRADEMANAGER_H

#include <QObject>
#include <QMap>
#include <QPointer>
#include <QDialog>

#include "ubuntuclicktool.h"

class QProcess;

namespace Ubuntu {
namespace Internal {

namespace Ui {
    class TargetUpgradeManagerDialog;
}

class TargetUpgradeManager : public QObject
{
    Q_OBJECT

    enum State {
        Idle,
        CollectPendingUpdates
    };

    struct Task {
        UbuntuClickTool::Target target;
        QPointer<QProcess> proc;
    };

public:
    explicit TargetUpgradeManager(QObject *parent = 0);

public slots:
    void checkForUpgrades ();

private slots:
    void processFinished ();

private:
    QMap<qintptr,Task> m_running;
    QList<UbuntuClickTool::Target> m_outdatedChroots;
    State m_state;

};

class TargetUpgradeManagerDialog : public QDialog
{
    Q_OBJECT

public:
    TargetUpgradeManagerDialog(QWidget *parent = 0);
    ~TargetUpgradeManagerDialog();

    static void selectAndUpgradeTargets (QList<UbuntuClickTool::Target> targets,QWidget *parent);

private:
    Ui::TargetUpgradeManagerDialog *m_ui;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_TARGETUPGRADEMANAGER_H
