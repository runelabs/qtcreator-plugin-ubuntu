#ifndef UBUNTU_INTERNAL_UBUNTUPACKAGESTEP_H
#define UBUNTU_INTERNAL_UBUNTUPACKAGESTEP_H

#include <projectexplorer/buildstep.h>
#include <projectexplorer/abstractprocessstep.h>

namespace ProjectExplorer{
class ToolChain;
}

namespace Ui {
class UbuntuPackageStepConfigWidget;
}

namespace Ubuntu {
namespace Internal {

class UbuntuPackageStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    Q_PROPERTY(PackageMode packageMode READ packageMode WRITE setPackageMode NOTIFY packageModeChanged)
public:

    enum State {
        Idle,
        MakeInstall,
        PreparePackage,
        ClickBuild,
        ClickReview
    };

    enum FinishedCheckMode {
        CheckReturnCode,
        IgnoreReturnCode
    };

    enum PackageMode {
        AutoEnableDebugScript, //Deprecated
        EnableDebugScript,
        DisableDebugScript
    };

    UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuPackageStep(ProjectExplorer::BuildStepList *bsl, UbuntuPackageStep *other);
    virtual ~UbuntuPackageStep();

public:
    // BuildStep interface
    virtual bool init() override;
    virtual void run(QFutureInterface<bool> &fi) override;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    virtual bool immutable() const override;
    virtual bool runInGuiThread() const override;

    // ProjectConfiguration interface
    virtual bool fromMap(const QVariantMap &map) override;
    virtual QVariantMap toMap() const override;

    QString packagePath () const;

    PackageMode packageMode() const;
    void setPackageMode(PackageMode arg);

signals:
    void packageModeChanged(PackageMode arg);

protected:
    void setupAndStartProcess ( const ProjectExplorer::ProcessParameters &params );
    bool processFinished (FinishedCheckMode mode = CheckReturnCode);
    void cleanup ();
    void stdOutput ( const QString &line );
    void stdError  ( const QString &line );
    QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const;

protected slots:
    void doNextStep ();
    void injectDebugHelperStep ();

    void onProcessStdOut ();
    void onProcessStdErr ();
    void onProcessFailedToStart ();

    void outputAdded(const QString &string, ProjectExplorer::BuildStep::OutputFormat format);
    void taskAdded (const ProjectExplorer::Task & task);

private:
    State m_state;
    QString m_lastLine;
    QString m_clickPackageName;
    QString m_buildDir;
    QList<ProjectExplorer::Task> m_tasks;
    QFutureInterface<bool> *m_futureInterface;

    ProjectExplorer::ProcessParameters m_MakeParam;
    ProjectExplorer::ProcessParameters m_ClickParam;
    ProjectExplorer::ProcessParameters m_ReviewParam;

    Utils::QtcProcess *m_process;
    ProjectExplorer::IOutputParser *m_outputParserChain;
    PackageMode m_packageMode;
};

class UbuntuPackageStepConfigWidget : public ProjectExplorer::SimpleBuildStepConfigWidget
{
    Q_OBJECT
public:
    UbuntuPackageStepConfigWidget(UbuntuPackageStep *step);
    ~UbuntuPackageStepConfigWidget();

    // BuildStepConfigWidget interface
    virtual bool showWidget() const;

public slots:
    void updateMode ();
    void onModeSelected (const int index);

private:
    Ui::UbuntuPackageStepConfigWidget *ui;
    bool m_isUpdating;


};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPACKAGESTEP_H
