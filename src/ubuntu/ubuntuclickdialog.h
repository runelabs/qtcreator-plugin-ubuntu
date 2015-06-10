/*
 * Copyright 2014 Canonical Ltd.
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
#ifndef UBUNTU_INTERNAL_UBUNTUCLICKDIALOG_H
#define UBUNTU_INTERNAL_UBUNTUCLICKDIALOG_H

#include <QDialog>
#include <QList>

#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>
#include "ubuntuclicktool.h"

namespace ProjectExplorer {
    class Project;
    class Target;
}

namespace Ubuntu {
namespace Internal {

namespace Ui {
    class UbuntuClickDialog;
}

class UbuntuClickDialog : public QDialog
{
    Q_OBJECT
public:
    UbuntuClickDialog (QWidget* parent = 0);
    ~UbuntuClickDialog ();

    void setParameters (const QList<ProjectExplorer::ProcessParameters> &params);
    int lastExitCode () const;


public slots:
    void runClick ();

    static int runClickModal(const ProjectExplorer::ProcessParameters &params, QWidget *parent = 0);
    static int runClickModal (const QList<ProjectExplorer::ProcessParameters> &params, QWidget *parent = 0);
    static bool createClickChrootModal (bool redetectKits = true , const QString &arch = QString(), const QString &framework = QString(), QWidget *parent = 0);

    static int maintainClickModal (const UbuntuClickTool::Target &target, const UbuntuClickTool::MaintainMode &mode);
    static int maintainClickModal (const QList<UbuntuClickTool::Target> &targetList, const UbuntuClickTool::MaintainMode &mode);

    // QDialog interface
    virtual void done(int code);

protected:
    void disableCloseButton (const bool &disabled = true);
    void nextTask ();

protected slots:
    void on_clickFinished(int exitCode);
    void on_clickReadyReadStandardOutput(const QString txt = QString());
    void on_clickReadyReadStandardError(const QString txt = QString());
private:
    Utils::QtcProcess *m_process;
    Ui::UbuntuClickDialog *m_ui;
    QList<ProjectExplorer::ProcessParameters> m_tasks;
    int m_exitCode;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCLICKDIALOG_H
