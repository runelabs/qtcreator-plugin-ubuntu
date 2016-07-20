#ifndef UBUNTU_INTERNAL_UBUNTUREMOTEANALYZESUPPORT_H
#define UBUNTU_INTERNAL_UBUNTUREMOTEANALYZESUPPORT_H

#include <QObject>
#include "abstractremoterunsupport.h"
#include <debugger/analyzer/analyzerruncontrol.h>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteRunConfiguration;
class UbuntuRemoteAnalyzeSupportPrivate;

class UbuntuRemoteAnalyzeSupport : public AbstractRemoteRunSupport
{
    Q_OBJECT
public:
    UbuntuRemoteAnalyzeSupport(UbuntuRemoteRunConfiguration *runConfig,
            Debugger::AnalyzerRunControl *engine, Core::Id runMode);
    ~UbuntuRemoteAnalyzeSupport() override;

protected:
    void startExecution() override;
    void handleAdapterSetupFailed(const QString &error) override;

private slots:
    void handleRemoteSetupRequested() override;
    void handleAppRunnerError(const QString &error) override;
    void handleRemoteOutput(const QByteArray &output) override;
    void handleRemoteErrorOutput(const QByteArray &output) override;
    void handleAppRunnerFinished(bool success) override;
    void handleProgressReport(const QString &progressOutput) override;

    void handleRemoteProcessStarted();
    void handleProfilingFinished();

    void remoteIsRunning();

private:
    void showMessage(const QString &, Utils::OutputFormat);

    UbuntuRemoteAnalyzeSupportPrivate * const d;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUREMOTEANALYZESUPPORT_H
