#include "containerdevice.h"
#include "containerdevice_p.h"
#include "containerdeviceprocess.h"

#include <ubuntu/ubuntuconstants.h>
#include <ubuntu/settings.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/taskhub.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace Ubuntu {
namespace Internal {

ContainerDevicePrivate::ContainerDevicePrivate(ContainerDevice *q)
    : QObject(nullptr)
    , q_ptr(q)
    , m_detectionProcess(nullptr)
{

}

void ContainerDevicePrivate::resetProcess()
{
    if (m_detectionProcess) {
        m_detectionProcess->disconnect(this);
        if (m_detectionProcess->state() != QProcess::NotRunning)
            m_detectionProcess->kill();
        delete m_detectionProcess;
        m_detectionProcess = nullptr;
    }
    m_detectionProcess = new QProcess(this);
    connect(m_detectionProcess, SIGNAL(finished(int)), this, SLOT(handleDetectionStepFinished()));
}

QString ContainerDevicePrivate::userName() const
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
        return QString::fromLatin1(pw->pw_name);
    return QString();
}

void ContainerDevicePrivate::reset()
{
    Q_Q(ContainerDevice);

    m_deviceState = Initial;
    ProjectExplorer::DeviceManager::instance()->setDeviceState(q->id(), ProjectExplorer::IDevice::DeviceDisconnected);

    handleDetectionStepFinished();
}

void ContainerDevicePrivate::showWarningMessage(const QString &msg)
{
    ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Error, msg, Constants::UBUNTU_TASK_CATEGORY_DEVICE);
}

void ContainerDevicePrivate::handleDetectionStepFinished()
{
    Q_Q(ContainerDevice);

    switch(m_deviceState) {
        case Initial: {
            m_deviceState = GetStatus;

            resetProcess();

            QString tool = Constants::UBUNTU_TARGET_TOOL;
            if (tool.isEmpty()) {
                showWarningMessage(tr("Could not find usdk-target in PATH.\n Make sure ubuntu-sdk-tools is installed."));
                return;
            }

            m_detectionProcess->setProgram(tool);
            m_detectionProcess->setArguments(QStringList{
                QStringLiteral("status"),
                q->containerName()
            });
            break;
        }
        case GetStatus: {
            if ((m_detectionProcess->exitStatus() != QProcess::NormalExit || m_detectionProcess->exitStatus() != 0)) {
                printProcessError();
                resetProcess();
                return;
            }

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson( m_detectionProcess->readAllStandardOutput(), &err);
            if (err.error != QJsonParseError::NoError) {
                showWarningMessage(tr("There was a error in the device detection of %1, it was not possible to parse the status:\n%2")
                                   .arg(q->containerName())
                                   .arg(err.errorString()));
                return;
            }

            if (!doc.isObject()) {
                showWarningMessage(tr("There was a error in the device detection of %1, the returned format was not a JSON object.")
                                   .arg(q->containerName()));
            }

            QVariantMap obj = doc.object().toVariantMap();
            if (!obj.contains(QStringLiteral("ipv4"))) {
                showWarningMessage(tr("There was a error in the device detection of %1, no IP address was returned.")
                                   .arg(q->containerName()));
                return;
            }

            m_deviceIP = obj[QStringLiteral("ipv4")].toString();
            m_deviceState = DeployKey;

            resetProcess();
            m_detectionProcess->setProgram(QString::fromLatin1(Constants::UBUNTU_CONTAINER_DEPLOY_PUBKEY_SCRIPT)
                                           .arg(Constants::UBUNTU_SCRIPTPATH));
            m_detectionProcess->setArguments(QStringList{q->containerName()});
            break;
        }
        case DeployKey: {
            if ((m_detectionProcess->exitStatus() != QProcess::NormalExit || m_detectionProcess->exitStatus() != 0)) {
                printProcessError();
                resetProcess();
                return;
            }

            m_deviceState = Finished;

            QSsh::SshConnectionParameters params = q->sshParameters();
            params.userName = userName();
            params.host = m_deviceIP;
            params.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
            params.timeout = 20;
            params.port = 22;
            params.privateKeyFile = Settings::settingsPath()
                    .appendPath(QLatin1String(Constants::UBUNTU_DEVICE_SSHIDENTITY))
                    .toString();

            q->setSshParameters(params);
            ProjectExplorer::DeviceManager::instance()->setDeviceState(q->id(), ProjectExplorer::IDevice::DeviceReadyToUse);
            return;
        }
        default: {
            break;
        }
    }

    m_detectionProcess->start();
    if (!m_detectionProcess->waitForStarted(3000)) {
        showWarningMessage(tr("Error while detecting the device state of %1.\n%2 %3")
                           .arg(q->containerName())
                           .arg(m_detectionProcess->program())
                           .arg(m_detectionProcess->arguments().join(QStringLiteral(" "))));
        resetProcess();
    }
}

void ContainerDevicePrivate::printProcessError()
{
    QString message = tr("There was a error in the device detection, it will not be possible to run apps on it:\n%1\n%2")
            .arg(QString::fromLocal8Bit(m_detectionProcess->readAllStandardOutput()))
            .arg(QString::fromLocal8Bit(m_detectionProcess->readAllStandardError()));
    showWarningMessage(message);
}

ContainerDevice::ContainerDevice(Core::Id type, Core::Id id) :
    LinuxDevice(type.suffixAfter(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID),
                type,
                ProjectExplorer::IDevice::Hardware,
                ProjectExplorer::IDevice::AutoDetected,
                id),
    d_ptr(new ContainerDevicePrivate(this))
{
    setDisplayName(QCoreApplication::translate("Ubuntu::Internal::ContainerDevice"
                                               , "Ubuntu Desktop Device (%1)").arg(containerName()));
    setDeviceState(IDevice::DeviceDisconnected);

    const QString portRange =
            QString::fromLatin1("%1-%2")
            .arg(Constants::UBUNTU_DESKTOP_PORT_START)
            .arg(Constants::UBUNTU_DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));

    d_ptr->reset();
}

ContainerDevice::ContainerDevice(const ContainerDevice &other)
    : LinuxDevice(other)
    , d_ptr(new ContainerDevicePrivate(this))
{
    //no need to copy over the private, just redetect the device status
    d_ptr->reset();
}

ContainerDevice::Ptr ContainerDevice::create(Core::Id type, Core::Id id)
{
    return ContainerDevice::Ptr(new ContainerDevice(type, id));
}

ContainerDevice::~ContainerDevice()
{
    delete d_ptr;
}

Core::Id ContainerDevice::createIdForContainer(const QString &name)
{
    return Core::Id(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID).withSuffix(name);
}

QString ContainerDevice::containerName() const
{
    return id().suffixAfter(Constants::UBUNTU_CONTAINER_DEVICE_TYPE_ID);
}

ProjectExplorer::IDeviceWidget *ContainerDevice::createWidget()
{
    if (!qgetenv("USDK_SHOW_DEVICE_WIDGET").isEmpty())
        return LinuxDevice::createWidget();
    return 0;
}

QList<Core::Id> ContainerDevice::actionIds() const
{
    return QList<Core::Id>();
}

void ContainerDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
}

ProjectExplorer::IDevice::Ptr ContainerDevice::clone() const
{
    return IDevice::Ptr(new ContainerDevice(*this));
}

QString ContainerDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

QString ContainerDevice::displayType() const
{
    return tr("Ubuntu Desktop Device");
}

ProjectExplorer::DeviceProcess *ContainerDevice::createProcess(QObject *parent) const
{
    return new ContainerDeviceProcess(sharedFromThis(), parent);
}

} // namespace Internal
} // namespace Ubuntu

