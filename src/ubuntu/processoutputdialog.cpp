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
#include "processoutputdialog.h"
#include "ui_processoutputdialog.h"

#include <texteditor/fontsettings.h>
#include <coreplugin/icore.h>

#include <QPushButton>
#include <QDebug>


namespace Ubuntu {
namespace Internal {

const char PROCESS_ERROR_EXIT_MESSAGE[] = "Task exited with errors, please check the output";
const char PROCESS_SUCCESS_EXIT_MESSAGE[] = "Task exited with no errors";

ProcessOutputDialog::ProcessOutputDialog(QWidget *parent)
    : QDialog(parent)
    ,m_ui(new Ui::ProcessOutputDialog)
{
    m_ui->setupUi(this);

    QFont f(TextEditor::FontSettings::defaultFixedFontFamily());
    f.setStyleHint(QFont::TypeWriter);
    m_ui->output->setFont(f);

    connect(m_ui->checkBox, &QCheckBox::toggled, [this](bool checked){
        m_ui->output->setVisible(checked);
        if(checked)
            adjustSize();
        else
            resize(minimumSize());
    });

    m_ui->checkBox->setChecked(false);

    m_process = new Utils::QtcProcess(this);
    connect(m_process,SIGNAL(readyReadStandardOutput()),this,SLOT(on_processReadyReadStandardOutput()));
    connect(m_process,SIGNAL(readyReadStandardError()),this,SLOT(on_processReadyReadStandardError()));
    connect(m_process,SIGNAL(finished(int)),this,SLOT(on_processFinished(int)));

    //make sure the progressbar is not animating just yet
    m_ui->progressBar->setRange(0, 1);
}

ProcessOutputDialog::~ProcessOutputDialog()
{
    delete m_ui;
}

void ProcessOutputDialog::setParameters(const QList<ProjectExplorer::ProcessParameters> &params)
{
    m_tasks = params;
}

int ProcessOutputDialog::lastExitCode() const
{
    return m_exitCode;
}

int ProcessOutputDialog::runProcessModal(const ProjectExplorer::ProcessParameters &params, QWidget *parent)
{
    return runProcessModal(QList<ProjectExplorer::ProcessParameters>()<<params,parent);
}

int ProcessOutputDialog::runProcessModal(const QList<ProjectExplorer::ProcessParameters> &params, QWidget *parent)
{
    ProcessOutputDialog dlg( parent ? parent : Core::ICore::mainWindow());
    dlg.setParameters(params);
    QMetaObject::invokeMethod(&dlg,"runTasks",Qt::QueuedConnection);
    dlg.exec();

    return dlg.m_exitCode;
}

void ProcessOutputDialog::runTasks( )
{
    m_hadErrors = false;
    disableCloseButton(true);
    nextTask();
}

void ProcessOutputDialog::done(int code)
{
    QDialog::done(code);
}

void ProcessOutputDialog::disableCloseButton(const bool &disabled)
{
    QPushButton* bt = m_ui->buttonBox->button(QDialogButtonBox::Close);
    if(bt) bt->setDisabled(disabled);
}

void ProcessOutputDialog::nextTask()
{
    if(m_tasks.length() <= 0)
        return;


    m_ui->progressBar->setRange(0,0);

    ProjectExplorer::ProcessParameters params = m_tasks.takeFirst();
    params.resolveAll();
    m_process->setCommand(params.command(),params.arguments());
    m_process->setEnvironment(params.environment());
    m_process->setWorkingDirectory(params.workingDirectory());
    m_process->start();
}

void ProcessOutputDialog::on_processFinished(int exitCode)
{
    if (exitCode != 0) {
        m_hadErrors = true;
        on_processReadyReadStandardError(tr("---%0---").arg(QLatin1String(PROCESS_ERROR_EXIT_MESSAGE)));
    } else {
        on_processReadyReadStandardOutput(tr("---%0---").arg(QLatin1String(PROCESS_SUCCESS_EXIT_MESSAGE)));
    }

    if(m_tasks.length() > 0) {
        nextTask();
        return;
    }

    m_ui->progressBar->setRange(0,1);
    m_ui->progressBar->setValue(1);
    disableCloseButton(false);
    m_exitCode = exitCode;

    if (m_hadErrors) {
        m_ui->label->setText(tr("There were errors while executing the tasks, please check the details."));
        m_ui->checkBox->setChecked(true);
    } else {
        m_ui->label->setText(tr("All tasks finished, check the details for more information"));
    }
}

void ProcessOutputDialog::on_processReadyReadStandardOutput(const QString txt)
{
    QString outText = QString::fromLocal8Bit("<div>");

    if(txt.isEmpty())
        outText.append(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    else
        outText.append(txt);

    outText.append(QString::fromLocal8Bit("</div>"));
    m_ui->output->append(outText);
}

void ProcessOutputDialog::on_processReadyReadStandardError(const QString txt)
{
    QString outText = QString::fromLocal8Bit("<div style=\"color:red; font-weight: bold;\">");

    if(txt.isEmpty())
        outText.append(QString::fromLocal8Bit(m_process->readAllStandardError()));
    else
        outText.append(txt);

    outText.append(QString::fromLocal8Bit("</div>"));
    m_ui->output->append(outText);
}

} // namespace Internal
} // namespace Ubuntu
