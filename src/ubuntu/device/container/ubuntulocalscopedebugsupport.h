#ifndef UBUNTU_INTERNAL_UBUNTULOCALSCOPEDEBUGSUPPORT_H
#define UBUNTU_INTERNAL_UBUNTULOCALSCOPEDEBUGSUPPORT_H

#include <projectexplorer/applicationlauncher.h>
#include <remotelinux/abstractremotelinuxrunsupport.h>

#include <QObject>

namespace Debugger{ class DebuggerRunControl; }


namespace Ubuntu {
namespace Internal {

class UbuntuLocalRunConfiguration;

class UbuntuLocalScopeDebugSupport : public RemoteLinux::AbstractRemoteLinuxRunSupport
{
    Q_OBJECT
public:
    explicit UbuntuLocalScopeDebugSupport(UbuntuLocalRunConfiguration *runConfig,
                                          Debugger::DebuggerRunControl *runControl,
                                          const QString &scopeRunnerPath);
    virtual ~UbuntuLocalScopeDebugSupport();

    // AbstractRemoteLinuxRunSupport interface
    virtual void startExecution() override;

protected slots:
    void handleRemoteSetupRequested();
    void handleAppRunnerError(const QString &error);
    void handleRemoteOutput(const QByteArray &output);
    void handleAppRunnerFinished(bool success);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleProgressReport(const QString &progressOutput);

protected:
    void showMessage(const QString &msg, int channel);

private:
    int m_port;
    QString m_scopeRunnerPath;
    QString m_executable;
    QStringList m_commandLineArguments;
    Debugger::DebuggerRunControl *m_runControl;
    QByteArray m_gdbserverOutput;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTULOCALSCOPEDEBUGSUPPORT_H
