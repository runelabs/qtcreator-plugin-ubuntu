#ifndef UBUNTUREMOTERUNNER_H
#define UBUNTUREMOTERUNNER_H

#include "ubuntudevice.h"

#include <QObject>
#include <QProcess>
#include <utils/environment.h>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteClickApplicationRunnerPrivate;

class UbuntuRemoteClickApplicationRunner : public QObject
{
    Q_OBJECT
public:

    enum CleanupMode{
        CleanSettings,
        KeepSettings
    };

    explicit UbuntuRemoteClickApplicationRunner(QObject *parent = 0);
    virtual ~UbuntuRemoteClickApplicationRunner();

    void start (UbuntuDevice::ConstPtr device, const QString &clickPackageName , const QString &hook);
    void stop  ();

    quint16 cppDebugPort() const;
    void setCppDebugPort(const quint16 &cppDebugPort);

    quint16 qmlDebugPort() const;
    void setQmlDebugPort(const quint16 &qmlDebugPort);

    Utils::Environment env() const;
    void setEnv(const Utils::Environment &env);

    void setForceInstall (const bool set);
    void setUninstall    (const bool set);

protected:
    void cleanup (CleanupMode mode = CleanSettings);

signals:
    void launcherStdout(const QByteArray &output);
    void launcherStderr(const QByteArray &output);
    void reportError(const QString &errorOutput);
    void launcherProcessStarted(quint16 pid);
    void clickApplicationStarted(quint16 pid);
    void finished(bool success);

private slots:
    void handleLauncherProcessError(QProcess::ProcessError error);
    void handleLauncherProcessFinished();
    void handleLauncherStdOut();
    void handleLauncherStdErr();

private:
    UbuntuRemoteClickApplicationRunnerPrivate *d;
};

}}

#endif // UBUNTUREMOTERUNNER_H
