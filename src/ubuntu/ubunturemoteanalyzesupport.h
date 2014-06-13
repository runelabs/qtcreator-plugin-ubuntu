#ifndef UBUNTU_INTERNAL_UBUNTUREMOTEANALYZESUPPORT_H
#define UBUNTU_INTERNAL_UBUNTUREMOTEANALYZESUPPORT_H

#include <QObject>
#include "abstractremoterunsupport.h"
#include <analyzerbase/analyzerruncontrol.h>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteRunConfiguration;
class UbuntuRemoteAnalyzeSupportPrivate;

class UbuntuRemoteAnalyzeSupport : public AbstractRemoteRunSupport
{
    Q_OBJECT
public:
    UbuntuRemoteAnalyzeSupport(UbuntuRemoteRunConfiguration *runConfig,
            Analyzer::AnalyzerRunControl *engine, ProjectExplorer::RunMode runMode);
    ~UbuntuRemoteAnalyzeSupport();

protected:
    void startExecution();
    void handleAdapterSetupFailed(const QString &error);

private slots:
    void handleRemoteSetupRequested();
    void handleAppRunnerError(const QString &error);
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);
    void handleAppRunnerFinished(bool success);
    void handleProgressReport(const QString &progressOutput);

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
