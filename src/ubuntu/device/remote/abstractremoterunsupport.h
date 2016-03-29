#ifndef UBUNTU_INTERNAL_ABSTRACTREMOTERUNSUPPORT_H
#define UBUNTU_INTERNAL_ABSTRACTREMOTERUNSUPPORT_H

#include <QObject>
#include <projectexplorer/devicesupport/idevice.h>
#include <utils/environment.h>

namespace Ubuntu {
namespace Internal {

class AbstractRemoteRunSupportPrivate;
class UbuntuRemoteRunConfiguration;
class UbuntuRemoteClickApplicationRunner;

class AbstractRemoteRunSupport : public QObject
{
    Q_OBJECT
public:

    enum State {
        Idle,
        ScanningPorts,
        Starting,
        Running
    };


    explicit AbstractRemoteRunSupport(UbuntuRemoteRunConfiguration* runConfig, QObject *parent = 0);
    virtual ~AbstractRemoteRunSupport();

protected:
    virtual void startExecution() = 0;

    virtual void handleAdapterSetupFailed(const QString &error);
    virtual void handleAdapterSetupDone();

    bool assignNextFreePort(int *port);
    void setFinished ();
    void reset();

    State state() const;
    void setState(const State &state);

    ProjectExplorer::IDevice::ConstPtr device () const;
    QString clickPackage () const;
    QString hook () const;
    Utils::Environment environment () const;

    UbuntuRemoteClickApplicationRunner *appRunner () const;

protected slots:
    virtual void handleRemoteSetupRequested();
    virtual void handleAppRunnerError(const QString &error) = 0;
    virtual void handleRemoteOutput(const QByteArray &output) = 0;
    virtual void handleRemoteErrorOutput(const QByteArray &output) = 0;
    virtual void handleAppRunnerFinished(bool success) = 0;
    virtual void handleProgressReport(const QString &progressOutput) = 0;

private slots:
    void handlePortScannerReady ();
    void handlePortScannerError ( const QString &message );


private:
    AbstractRemoteRunSupportPrivate * const d;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_ABSTRACTREMOTERUNSUPPORT_H
