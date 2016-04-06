#ifndef UBUNTU_INTERNAL_UBUNTULOCALSCOPEDEBUGSUPPORT_H
#define UBUNTU_INTERNAL_UBUNTULOCALSCOPEDEBUGSUPPORT_H

#include <projectexplorer/applicationlauncher.h>
#include <debugger/debuggerruncontrol.h>

#include <QObject>

namespace Debugger{ class DebuggerRunControl; }


namespace Ubuntu {
namespace Internal {

class UbuntuLocalRunConfiguration;

class UbuntuLocalScopeDebugSupport : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuLocalScopeDebugSupport(UbuntuLocalRunConfiguration *runConfig,
                                          Debugger::DebuggerRunControl *runControl,
                                          const QString &scopeRunnerPath);
    virtual ~UbuntuLocalScopeDebugSupport();

    ProjectExplorer::ApplicationLauncher::Mode appLauncherMode() const;
    void setAppLauncherMode(const ProjectExplorer::ApplicationLauncher::Mode &appLauncherMode);

private:
    quint16 getLocalPort() const;

private slots:
    void handleRemoteSetupRequested();
    void handleProcessStarted();
    void handleProcessExited(int exitCode, QProcess::ExitStatus);
    void handleError(QProcess::ProcessError error);
    void handleStateChanged(Debugger::DebuggerState state);

private:
    quint16 m_port;
    QString m_scopeRunnerPath;
    QString m_executable;
    QString m_commandLineArguments;
    Debugger::DebuggerRunControl *m_runControl;
    ProjectExplorer::ApplicationLauncher m_launcher;
    ProjectExplorer::ApplicationLauncher::Mode m_appLauncherMode;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTULOCALSCOPEDEBUGSUPPORT_H
