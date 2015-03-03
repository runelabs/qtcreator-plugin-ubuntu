#ifndef UBUNTU_INTERNAL_UBUNTUREMOTEDEBUGSUPPORT_H
#define UBUNTU_INTERNAL_UBUNTUREMOTEDEBUGSUPPORT_H

#include "abstractremoterunsupport.h"

#include <debugger/debuggerengine.h>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteDebugSupportPrivate;
class UbuntuRemoteRunConfiguration;

class UbuntuRemoteDebugSupport : public AbstractRemoteRunSupport
{
    Q_OBJECT
public:
    UbuntuRemoteDebugSupport(UbuntuRemoteRunConfiguration *runConfig,
            Debugger::DebuggerRunControl *runControl);
    ~UbuntuRemoteDebugSupport();

protected:
    void startExecution() override;
    void handleAdapterSetupFailed(const QString &error) override;


private slots:
    void handleRemoteSetupRequested() override;

    void handleAdapterSetupDone() override;
    void handleAppRunnerError(const QString &error) override;
    void handleRemoteOutput(const QByteArray &output) override;
    void handleRemoteErrorOutput(const QByteArray &output) override;
    void handleAppRunnerFinished(bool success) override;
    void handleProgressReport(const QString &progressOutput) override;

    void handleRemoteProcessStarted();
    void handleDebuggingFinished();

private:
    void showMessage(const QString &msg, int channel);

    UbuntuRemoteDebugSupportPrivate * const d;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUREMOTEDEBUGSUPPORT_H
