#ifndef UBUNTU_INTERNAL_UBUNTUREMOTEDEBUGSUPPORT_H
#define UBUNTU_INTERNAL_UBUNTUREMOTEDEBUGSUPPORT_H

#include <QObject>

#include <remotelinux/abstractremotelinuxrunsupport.h>
#include <debugger/debuggerengine.h>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteDebugSupportPrivate;
class UbuntuRemoteRunConfiguration;

class UbuntuRemoteDebugSupport : public RemoteLinux::AbstractRemoteLinuxRunSupport
{
    Q_OBJECT
public:
    UbuntuRemoteDebugSupport(UbuntuRemoteRunConfiguration *runConfig,
            Debugger::DebuggerEngine *engine);
    ~UbuntuRemoteDebugSupport();

protected:
    void startExecution();
    void handleAdapterSetupFailed(const QString &error);
    void handleAdapterSetupDone();

private slots:
    void handleRemoteSetupRequested();
    void handleAppRunnerError(const QString &error);
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleAppRunnerFinished(bool success);
    void handleProgressReport(const QString &progressOutput);

    void handleRemoteProcessStarted();
    void handleDebuggingFinished();

private:
    void showMessage(const QString &msg, int channel);

    UbuntuRemoteDebugSupportPrivate * const d;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUREMOTEDEBUGSUPPORT_H
