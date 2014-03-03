#include "ubuntudevice.h"
#include "ubuntudevicenotifier.h"
#include "ubuntuprocess.h"
#include "ubuntuconstants.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <QCoreApplication>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuDeviceHelper
 * Helper class that collects informations about the
 * device, and can be used to monitor connect/disconnect
 * and other device events
 */
UbuntuDeviceHelper::UbuntuDeviceHelper(UbuntuDevice *dev)
    : QObject(0)
    , m_dev(dev)
    , m_deviceWatcher(new UbuntuDeviceNotifier(this))
    , m_process(new UbuntuProcess(this))
{
    connect(m_deviceWatcher,SIGNAL(deviceConnected()),this,SLOT(deviceConnected()));
    connect(m_deviceWatcher,SIGNAL(deviceDisconnected()),this,SLOT(deviceDisconnected()));
    connect(m_process,SIGNAL(finished(QString,int)),this,SLOT(processFinished(QString,int)));
}

UbuntuDeviceHelper::~UbuntuDeviceHelper()
{
    if(m_deviceWatcher)
        m_deviceWatcher->stopMonitoring();
    if(m_process)
        m_process->stop();
}

UbuntuDevice *UbuntuDeviceHelper::device() const
{
    return m_dev;
}

void UbuntuDeviceHelper::init()
{
    if(!m_dev->serialNumber().isEmpty())
        m_deviceWatcher->startMonitoring(m_dev->id().toSetting().toString());

    if(m_deviceWatcher->isConnected()) {
        deviceConnected();
    }
}

void UbuntuDeviceHelper::processFinished(const QString &, const int code)
{
    Q_UNUSED(code)
    switch(m_dev->m_processState) {
        case UbuntuDevice::DetectDeviceVersion:{
            //seems to do just nothing, devicewidget did not check return code
            //emit a message here
            detectHasNetworkConnection();
            break;
        }
        case UbuntuDevice::DetectNetworkConnection:{
            if (m_reply.trimmed() == QString::fromLatin1(Constants::ONE_STR)) {
                // we have network
                m_dev->m_hasNetworkConnection = UbuntuDevice::Available;
                detectOpenSsh();
                emit featureDetected();
            } else {
                // not set
                m_dev->m_hasNetworkConnection = UbuntuDevice::NotAvailable;
                m_dev->m_processState = UbuntuDevice::Done; //for now
                emit featureDetected();
                emit deviceNeedsSetup();
            }
            break;
        }
        case UbuntuDevice::DetectOpenSSH:{
            if (m_reply.trimmed() != QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NONE)
                    && m_reply.trimmed() != QLatin1String(Constants::EMPTY)) {
                m_dev->m_hasOpenSSHServer = UbuntuDevice::Available;
                startSshService();
                emit featureDetected();
            } else {
                m_dev->m_hasOpenSSHServer = UbuntuDevice::NotAvailable;
                m_dev->m_processState = UbuntuDevice::Done; //for now
                emit featureDetected();
                emit deviceNeedsSetup();
            }
            break;
        }
        case UbuntuDevice::InstallOpenSSH:{
            detectOpenSsh();
            break;
        }
        case UbuntuDevice::StartOpenSSH:{
            m_dev->m_openSSHStarted = UbuntuDevice::Available;
            emit featureDetected();

            m_dev->m_processState = UbuntuDevice::EnablePortForwarding;
            //@TODO per device settings
            QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
            settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
            QString deviceQmlPort = settings.value(QLatin1String(Constants::SETTINGS_KEY_QML),Constants::SETTINGS_DEFAULT_DEVICE_QML_PORT).toString();
            QString deviceSshPort = settings.value(QLatin1String(Constants::SETTINGS_KEY_SSH),Constants::SETTINGS_DEFAULT_DEVICE_SSH_PORT).toString();

            m_process->stop();
            m_process->append(QStringList()
                              << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_PORTFORWARD_SCRIPT)
                              .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                              .arg(m_dev->id().toSetting().toString())
                              .arg(deviceSshPort)
                              .arg(deviceQmlPort)
                              << QCoreApplication::applicationDirPath());
            m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_PORTFORWARD));
            break;
        }
        case UbuntuDevice::EnablePortForwarding:{
            m_dev->m_processState = UbuntuDevice::DeployPublicKey;

            QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
            settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
            QString deviceUsername = settings.value(QLatin1String(Constants::SETTINGS_KEY_USERNAME),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_USERNAME)).toString();

            m_process->stop();
            m_process->append(QStringList()
                              << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH_SCRIPT)
                              .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                              .arg(m_dev->id().toSetting().toString())
                              .arg(deviceUsername)
                              << QCoreApplication::applicationDirPath());
            m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH));
            break;
        }
        case UbuntuDevice::DeployPublicKey:{
            //ready to use
            ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceReadyToUse);

            detectDeviceWritableImage();
            break;
        }
        case UbuntuDevice::DetectDeviceWriteableImage:{
            if (m_reply.trimmed() == QLatin1String(Constants::ZERO_STR)) {
                m_dev->m_hasWriteableImage = UbuntuDevice::Available;
            } else {
                m_dev->m_hasWriteableImage = UbuntuDevice::NotAvailable;
            }
            emit featureDetected();
            detectDeveloperTools();
            break;
        }
        case UbuntuDevice::DetectDeveloperTools:{
            m_dev->m_processState = UbuntuDevice::Done;
            m_process->stop();

            if (m_reply.trimmed() == QLatin1String(Constants::ZERO_STR))
                m_dev->m_hasDeveloperTools = UbuntuDevice::NotAvailable;
            else
                m_dev->m_hasDeveloperTools = UbuntuDevice::Available;

            emit featureDetected();
            break;
        }
        default:{
            break;
        }
    }
    m_reply.clear();
}

void UbuntuDeviceHelper::onMessage(QString msg) {
    m_reply.append(msg);

    emit message(msg);
}

void UbuntuDeviceHelper::detect()
{
    //start feature detection
    detectDeviceVersion();
}

void UbuntuDeviceHelper::detectOpenSsh()
{
    m_dev->m_processState = UbuntuDevice::DetectOpenSSH;
    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTOPENSSH_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(m_dev->id().toSetting().toString())
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTOPENSSH));
}

void UbuntuDeviceHelper::startSshService()
{
    m_dev->m_processState = UbuntuDevice::StartOpenSSH;

    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(m_dev->id().toSetting().toString())
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE));
}

void UbuntuDeviceHelper::enableDeveloperMode()
{
    m_dev->m_processState = UbuntuDevice::InstallOpenSSH;

    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(m_dev->id().toSetting().toString())
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL));
}

void UbuntuDeviceHelper::detectHasNetworkConnection()
{
    m_dev->m_processState = UbuntuDevice::DetectNetworkConnection;
    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_HASNETWORK_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(m_dev->id().toSetting().toString())
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_HASNETWORK));
}

void UbuntuDeviceHelper::detectDeviceVersion()
{
    m_dev->m_processState = UbuntuDevice::DetectDeviceVersion;
    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(m_dev->id().toSetting().toString())
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION));
}

void UbuntuDeviceHelper::detectDeviceWritableImage()
{
    m_dev->m_processState = UbuntuDevice::DetectDeviceWriteableImage;

    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(m_dev->id().toSetting().toString())
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE));
}

void UbuntuDeviceHelper::detectDeveloperTools()
{
    m_dev->m_processState = UbuntuDevice::DetectDeveloperTools;

    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS_SCRIPT)
                      .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      .arg(m_dev->id().toSetting().toString())
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS));
}

void UbuntuDeviceHelper::deviceConnected()
{
    ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceConnected);
    detect();
}

void UbuntuDeviceHelper::deviceDisconnected()
{
    ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceDisconnected);
    resetToDefaults();
    emit disconnected();
}

void UbuntuDeviceHelper::resetToDefaults()
{
    m_reply.clear();
    m_dev->m_openSSHStarted       = UbuntuDevice::Unknown;
    m_dev->m_hasOpenSSHServer     = UbuntuDevice::Unknown;
    m_dev->m_hasNetworkConnection = UbuntuDevice::Unknown;
    m_dev->m_hasWriteableImage    = UbuntuDevice::Unknown;
    m_dev->m_hasDeveloperTools    = UbuntuDevice::Unknown;
}

void UbuntuDeviceHelper::beginAction(QString msg) {
    //ui->plainTextEdit->appendHtml(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_BEGIN).arg(msg));
}

void UbuntuDeviceHelper::endAction(QString msg) {
    //ui->plainTextEdit->appendHtml(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_END).arg(msg));
}

UbuntuDevice::UbuntuDevice()
    : LinuxDevice()
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
{
    setDeviceState(ProjectExplorer::IDevice::DeviceDisconnected);
    loadDefaultSSHParams();
}

UbuntuDevice::UbuntuDevice(const QString &name, ProjectExplorer::IDevice::MachineType machineType, ProjectExplorer::IDevice::Origin origin, Core::Id id)
    : LinuxDevice(name,Core::Id(Constants::UBUNTU_DEVICE_ID),machineType,origin,id)
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
{
    setDeviceState(ProjectExplorer::IDevice::DeviceDisconnected);
    loadDefaultSSHParams();
    m_helper->init();
}

//@TODO make sure this is required, we maybe don't want to clone a device at all
UbuntuDevice::UbuntuDevice(const UbuntuDevice &other)
    : LinuxDevice(other)
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
{
    setDeviceState(ProjectExplorer::IDevice::DeviceDisconnected);
    loadDefaultSSHParams();
}

void UbuntuDevice::loadDefaultSSHParams()
{
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));

    QString deviceSshPort = settings.value(QLatin1String(Constants::SETTINGS_KEY_SSH),Constants::SETTINGS_DEFAULT_DEVICE_SSH_PORT).toString();

    QSsh::SshConnectionParameters params;
    params.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
    params.host = QLatin1String("127.0.0.1");
    params.port = deviceSshPort.toUInt();
}

UbuntuDevice::~UbuntuDevice()
{
    delete m_helper;
}

UbuntuDevice::Ptr UbuntuDevice::create()
{
    return Ptr(new UbuntuDevice());
}

QString UbuntuDevice::serialNumber() const
{
    return id().toSetting().toString();
}

UbuntuDeviceHelper *UbuntuDevice::helper() const
{
    return m_helper;
}

bool UbuntuDevice::developerModeEnabled()
{
    return m_hasOpenSSHServer;
}

void UbuntuDevice::enableDeveloperMode()
{
    if(m_processState != Done || m_hasOpenSSHServer)
        return;

    m_helper->enableDeveloperMode();
}

UbuntuDevice::Ptr UbuntuDevice::create(const QString &name, const QString& serial, ProjectExplorer::IDevice::MachineType machineType, ProjectExplorer::IDevice::Origin origin)
{
    return Ptr(new UbuntuDevice(name,machineType,origin,Core::Id::fromSetting(serial)));
}

} // namespace Internal
} // namespace Ubuntu
