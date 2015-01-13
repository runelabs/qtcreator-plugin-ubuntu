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
        AptGetUpdate,
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
