#ifndef UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
#define UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H

#include <QList>
#include <QString>
#include <QDialog>
#include <QFutureInterface>
#include <QQueue>
#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>


class QDialogButtonBox;
class QPlainTextEdit;
class QLabel;
class QAction;

namespace ProjectExplorer {
    class Project;
    class Target;
}

namespace Ubuntu {
namespace Internal {

class UbuntuClickBuildConfiguration;

class UbuntuClickTool
{
public:

    enum MaintainMode {
        Upgrade,//runs click chroot upgrade
        Delete  //runs click chroot delete
    };

    struct Target {
        QString framework;
        QString architecture;
    };

    UbuntuClickTool();

    static void parametersForCreateChroot   (const QString &arch, const QString &series,ProjectExplorer::ProcessParameters* params);
    static void parametersForMaintainChroot (const MaintainMode &mode,const Target& target,ProjectExplorer::ProcessParameters* params);
    static void parametersForCmake        (const Target& target, const QString &buildDir
                                    , const QString &relPathToSource,ProjectExplorer::ProcessParameters* params);
    static void parametersForMake         (const Target& target, const QString &buildDir,bool doClean, ProjectExplorer::ProcessParameters* params);

    static void openChrootTerminal (const Target& target);

    static QList<Target> listAvailableTargets ();


private:
    Utils::QtcProcess *m_clickProcess;
};

class UbuntuClickDialog : public QDialog
{
    Q_OBJECT
public:
    UbuntuClickDialog (QWidget* parent = 0);
    ~UbuntuClickDialog ();

    void setParameters (ProjectExplorer::ProcessParameters* params);

public slots:
    void runClick ();

    static void runClickModal (ProjectExplorer::ProcessParameters* params);
    static void createClickChrootModal ();
    static void maintainClickModal (const UbuntuClickTool::Target &target, const UbuntuClickTool::MaintainMode &mode);

    // QDialog interface
    virtual void done(int code);

protected slots:
    void on_clickFinished(int exitCode);
    void on_clickReadyReadStandardOutput(const QString txt = QString());
    void on_clickReadyReadStandardError(const QString txt = QString());
private:
    QDialogButtonBox  *m_buttonBox;
    Utils::QtcProcess *m_process;
    QPlainTextEdit    *m_output;
    QLabel            *m_exitStatus;
};

class UbuntuClickManager : public QObject
{
    Q_OBJECT
public:

    enum BuildState {
        NotStarted,
        MakeClean,
        Cmake,
        FixMoc,
        Make,
        Finished
    };

    struct Build {
        ProjectExplorer::Target* buildTarget;
        UbuntuClickTool::Target  targetChroot;
        BuildState currentState;
        QString buildDir;
    };


    UbuntuClickManager (QObject* parent = 0);
    void initialize ();

protected:
    void startProcess (const ProjectExplorer::ProcessParameters& params);

public slots:
    void processBuildQueue ();
    void stop();

protected slots:
    void cleanup  ();
    void nextStep ();
    void updateSelectedProject (ProjectExplorer::Project *project);

    void on_buildInChrootAction();
    void on_processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void on_processReadyRead();
private:
    ProjectExplorer::Project *m_currentProject;
    QAction                  *m_buildInChrootAction;

    bool                    m_failOnError; //should we fail if the current step has errors?
    Utils::QtcProcess      *m_process;
    QFutureInterface<void> *m_futureInterface;
    Build                  *m_currentBuild;
    QQueue<Build*>          m_pendingBuilds;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
