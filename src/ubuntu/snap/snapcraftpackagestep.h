/*
 * Copyright 2016 Canonical Ltd.
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

#ifndef UBUNTU_INTERNAL_SNAPCRAFTPACKAGESTEP_H
#define UBUNTU_INTERNAL_SNAPCRAFTPACKAGESTEP_H


#include <projectexplorer/buildstep.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>

namespace ProjectExplorer{
class ToolChain;
}

namespace Ubuntu {
namespace Internal {

class SnapcraftPackageStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:

    enum State {
        Idle,
        MakeSnap,
        SnapReview
    };

    enum FinishedCheckMode {
        CheckReturnCode,
        IgnoreReturnCode
    };

    SnapcraftPackageStep(ProjectExplorer::BuildStepList *bsl);
    SnapcraftPackageStep(ProjectExplorer::BuildStepList *bsl, SnapcraftPackageStep *other);
    virtual ~SnapcraftPackageStep();

    QString packagePath () const;
    QString snapWorkingDir () const;

    // BuildStep interface
    virtual bool init(QList<const ProjectExplorer::BuildStep *> &earlierSteps) override;
    virtual void run(QFutureInterface<bool> &fi) override;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    virtual void cancel() override;
    virtual bool immutable() const override;
    virtual bool runInGuiThread() const override;

signals:
    void currentSubStepFinished();

protected slots:
    void doNextStep ();
    void onProcessFailedToStart();
    void onProcessStdOut();
    void onProcessStdErr();
    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded (const ProjectExplorer::Task & task);

private:
    void internalInit ();
    void cleanup();
    QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const;
    void setupAndStartProcess(ProjectExplorer::ProcessParameters &params);
    void stdOutput(const QString &line);
    void stdError(const QString &line);
    bool processFinished(FinishedCheckMode mode);

private:
    State m_state = SnapcraftPackageStep::Idle;
    QList<ProjectExplorer::Task> m_tasks;
    QFutureInterface<bool> *m_futureInterface;

    Utils::QtcProcess *m_process = nullptr;
    ProjectExplorer::IOutputParser *m_outputParserChain = nullptr;

    QString m_buildDir;
    QString m_lastLine;
    QString m_snapPackageName;
    ProjectExplorer::ProcessParameters m_MakeParam;
    ProjectExplorer::ProcessParameters m_SnapReviewParam;
    ProjectExplorer::ProcessParameters *m_currParam = nullptr;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_SNAPCRAFTPACKAGESTEP_H
