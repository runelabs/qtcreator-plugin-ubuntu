#include "abstractremoterunsupport.h"
#include "ubunturemoterunner.h"
#include "ubunturemoterunconfiguration.h"
#include "ubuntudevice.h"

#include <utils/qtcassert.h>
#include <utils/environment.h>
#include <utils/portlist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>


namespace Ubuntu {
namespace Internal {

class AbstractRemoteRunSupportPrivate
{
public:

    AbstractRemoteRunSupportPrivate( const UbuntuRemoteRunConfiguration *runConfig ) :
        clickPackage(runConfig->clickPackage()),
        hook(runConfig->appId()),
        dev(ProjectExplorer::DeviceKitInformation::device(runConfig->target()->kit())),
        env(runConfig->environment()),
        freePorts(dev->freePorts()),
        m_state(AbstractRemoteRunSupport::Idle){}

    QString clickPackage;
    QString hook;
    const ProjectExplorer::IDevice::ConstPtr dev;
    Utils::Environment env;
    Utils::PortList freePorts;
    UbuntuRemoteClickApplicationRunner runner;
    AbstractRemoteRunSupport::State m_state;
    ProjectExplorer::DeviceUsedPortsGatherer portScanner;
};

AbstractRemoteRunSupport::AbstractRemoteRunSupport(UbuntuRemoteRunConfiguration* runConfig, QObject *parent) :
    QObject(parent),
    d(new AbstractRemoteRunSupportPrivate(runConfig))
{
    d->runner.setForceInstall(runConfig->forceInstall());
    d->runner.setUninstall(runConfig->uninstall());
}

AbstractRemoteRunSupport::~AbstractRemoteRunSupport()
{
    delete d;
}

AbstractRemoteRunSupport::State AbstractRemoteRunSupport::state() const
{
    return d->m_state;
}

void AbstractRemoteRunSupport::setState(const State &state)
{
    d->m_state = state;
}

ProjectExplorer::IDevice::ConstPtr AbstractRemoteRunSupport::device() const
{
    return d->dev;
}

QString AbstractRemoteRunSupport::clickPackage() const
{
    return d->clickPackage;
}

QString AbstractRemoteRunSupport::hook() const
{
    return d->hook;
}

Utils::Environment AbstractRemoteRunSupport::environment() const
{
    return d->env;
}

UbuntuRemoteClickApplicationRunner *AbstractRemoteRunSupport::appRunner() const
{
    return &d->runner;
}

bool AbstractRemoteRunSupport::assignNextFreePort(int *port)
{
    *port = d->portScanner.getNextFreePort(&d->freePorts);
    if (*port == -1) {
        handleAdapterSetupFailed(tr("Not enough free ports on device for debugging."));
        return false;
    }
    return true;
}

void AbstractRemoteRunSupport::setFinished()
{
    if (d->m_state == Idle)
        return;
    if (d->m_state == Running)
        d->runner.stop();
    d->m_state = Idle;
}

void AbstractRemoteRunSupport::reset()
{
    d->portScanner.disconnect(this);
    d->runner.disconnect(this);
    d->m_state = Idle;
}

void AbstractRemoteRunSupport::handleRemoteSetupRequested()
{
    QTC_ASSERT(d->m_state == Idle, return);

    d->m_state = ScanningPorts;
    connect(&d->portScanner,SIGNAL(portListReady()),this,SLOT(handlePortScannerReady()));
    connect(&d->portScanner,SIGNAL(error(QString)),this,SLOT(handlePortScannerError(QString)));
    d->portScanner.start(d->dev);
}

void AbstractRemoteRunSupport::handleAdapterSetupDone()
{
    d->m_state = Running;
}

void AbstractRemoteRunSupport::handlePortScannerReady()
{
    QTC_ASSERT(d->m_state == ScanningPorts, return);

    d->freePorts = d->dev->freePorts();
    startExecution();
}

void AbstractRemoteRunSupport::handlePortScannerError(const QString &message)
{
    QTC_ASSERT( d->m_state == ScanningPorts , return );
    handleAdapterSetupFailed(message);
}

void AbstractRemoteRunSupport::handleAdapterSetupFailed(const QString &)
{
    setFinished();
    reset();
}


} // namespace Internal
} // namespace Ubuntu
