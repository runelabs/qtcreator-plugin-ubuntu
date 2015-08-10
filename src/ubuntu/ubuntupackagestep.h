#ifndef UBUNTU_INTERNAL_UBUNTUPACKAGESTEP_H
#define UBUNTU_INTERNAL_UBUNTUPACKAGESTEP_H

#include <projectexplorer/buildstep.h>
#include <projectexplorer/abstractprocessstep.h>

#include <QPointer>

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
    Q_PROPERTY(DebugMode debugMode READ debugMode WRITE setDebugMode NOTIFY packageModeChanged)
    Q_PROPERTY(bool treatClickErrorsAsWarnings READ treatClickErrorsAsWarnings WRITE setTreatClickErrorsAsWarnings NOTIFY treatClickErrorsAsWarningsChanged)
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

    enum DebugMode {
        AutoEnableDebugScript, //Deprecated
        EnableDebugScript,
        DisableDebugScript
    };

    enum PackageMode {
        Default,
        OnlyMakeInstall,
        OnlyClickBuild
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

    DebugMode debugMode() const;
    void setDebugMode(DebugMode arg);

    PackageMode packageMode() const;
    void setPackageMode(const PackageMode &packageMode);

    QString overrideInstallDir() const;
    void setOverrideDeployDir(const QString &overrideInstallDir);

    QString overrideClickWorkingDir() const;
    void setOverrideClickWorkingDir(const QString &overrideClickWorkingDir);

    QString clickWorkingDir() const;

    ProjectExplorer::BuildConfiguration *referenceBuildConfig() const;
    void setReferenceBuildConfig(ProjectExplorer::BuildConfiguration *referenceBuildConfig);

    bool cleanDeployDirectory() const;
    void setCleanDeployDirectory(bool cleanDeployDirectory);

    bool treatClickErrorsAsWarnings() const;
    void setTreatClickErrorsAsWarnings(bool arg);

signals:
    void packageModeChanged(DebugMode arg);
    void currentSubStepFinished();

    void treatClickErrorsAsWarningsChanged(bool arg);

protected:
    void internalInit ();
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
    QString m_deployDir;
    QList<ProjectExplorer::Task> m_tasks;
    QFutureInterface<bool> *m_futureInterface;

    ProjectExplorer::ProcessParameters m_MakeParam;
    ProjectExplorer::ProcessParameters m_ClickParam;
    ProjectExplorer::ProcessParameters m_ReviewParam;

    Utils::QtcProcess *m_process;
    ProjectExplorer::IOutputParser *m_outputParserChain;

    DebugMode m_debugMode;
    PackageMode m_packageMode;
    QString m_overrideDeployDir;
    QString m_overrideClickWorkingDir;
    QPointer<ProjectExplorer::BuildConfiguration> m_referenceBuildConfig;
    bool m_cleanDeployDirectory;
    bool m_treatClickErrorsAsWarnings;
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
    void onClickErrorsToggled (const bool checked);

private:
    Ui::UbuntuPackageStepConfigWidget *ui;
    bool m_isUpdating;


};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUPACKAGESTEP_H
