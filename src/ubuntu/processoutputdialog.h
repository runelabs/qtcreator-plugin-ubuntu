/*
 * Copyright 2014-2016 Canonical Ltd.
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
#ifndef UBUNTU_INTERNAL_PROCESSOUTPUTDIALOG_H
#define UBUNTU_INTERNAL_PROCESSOUTPUTDIALOG_H

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
    class ProcessOutputDialog;
}

class ProcessOutputDialog : public QDialog
{
    Q_OBJECT
public:
    ProcessOutputDialog (QWidget* parent = 0);
    ~ProcessOutputDialog ();

    void setParameters (const QList<ProjectExplorer::ProcessParameters> &params);
    int lastExitCode () const;


public slots:
    void runTasks ();

    static int runProcessModal(const ProjectExplorer::ProcessParameters &params, QWidget *parent = 0);
    static int runProcessModal (const QList<ProjectExplorer::ProcessParameters> &params, QWidget *parent = 0);

    // QDialog interface
    virtual void done(int code);

protected:
    void disableCloseButton (const bool &disabled = true);
    void nextTask ();

protected slots:
    void on_processFinished(int exitCode);
    void on_processReadyReadStandardOutput(const QString txt = QString());
    void on_processReadyReadStandardError(const QString txt = QString());
private:
    Utils::QtcProcess *m_process;
    Ui::ProcessOutputDialog *m_ui;
    QList<ProjectExplorer::ProcessParameters> m_tasks;
    int m_exitCode;
    bool m_hadErrors;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_PROCESSOUTPUTDIALOG_H
