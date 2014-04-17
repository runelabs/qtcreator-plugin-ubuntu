/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */
#include "ubuntudevice.h"
#include "ubuntudevicenotifier.h"
#include "ubuntuemulatornotifier.h"
#include "ubuntuprocess.h"
#include "ubuntuconstants.h"
#include "ubuntudeviceconfigurationwidget.h"
#include "localportsmanager.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <remotelinux/genericlinuxdeviceconfigurationwidget.h>
#include <coreplugin/messagemanager.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

namespace Ubuntu {
namespace Internal {

const QLatin1String DEVICE_INFO_KEY("UbuntuDevice.InfoString");

/*!
 * \class UbuntuDeviceHelper
 * Helper class that collects informations about the
 * device, and can be used to monitor connect/disconnect
 * and other device events
 */
UbuntuDeviceHelper::UbuntuDeviceHelper(UbuntuDevice *dev)
    : QObject(0)
    , m_clonedNwCount(0)
    , m_dev(dev)
    , m_process(0)
    , m_deviceWatcher(0)
{
#if 0
    connect(m_process,SIGNAL(finished(QString,int)),this,SLOT(processFinished(QString,int)));
    connect(m_process,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(m_process,SIGNAL(error(QString)),this,SLOT(onError(QString)));
#endif
    resetToDefaults();
}

UbuntuDeviceHelper::~UbuntuDeviceHelper()
{
    if(m_deviceWatcher)
        m_deviceWatcher->stopMonitoring();
    if(m_process)
        stopProcess();
}

UbuntuDevice *UbuntuDeviceHelper::device() const
{
    return m_dev;
}

QString UbuntuDeviceHelper::log() const
{
    return m_log;
}

void UbuntuDeviceHelper::refresh()
{
    deviceDisconnected();
    if(m_deviceWatcher && m_deviceWatcher->isConnected()) {
        deviceConnected();
    }
}

void UbuntuDeviceHelper::init()
{
    if(!m_dev->serialNumber().isEmpty()) {

        if(!m_deviceWatcher) {
            if(m_dev->serialNumber().startsWith(QLatin1String("emulator")))
                m_deviceWatcher = new UbuntuEmulatorNotifier(this);
            else
                m_deviceWatcher = new UbuntuDeviceNotifier(this);

            m_deviceWatcher->stopMonitoring();
            m_deviceWatcher->startMonitoring(m_dev->id().toSetting().toString());

            connect(m_deviceWatcher,SIGNAL(deviceConnected()),this,SLOT(deviceConnected()));
            connect(m_deviceWatcher,SIGNAL(deviceDisconnected()),this,SLOT(deviceDisconnected()));
        }
    }

    if(m_deviceWatcher && m_deviceWatcher->isConnected()) {
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
                m_clonedNwCount = 0;
                if(m_dev->m_hasNetworkConnection != UbuntuDevice::Available) {
                    m_dev->m_hasNetworkConnection = UbuntuDevice::Available;
                    emit featureDetected();
                }
                detectOpenSsh();

            } else {
                // not set
                if (m_clonedNwCount == 0)
                    cloneNetwork();
                else {
                    //we tried to enable network and failed
                    if(m_dev->m_hasNetworkConnection != UbuntuDevice::NotAvailable) {
                        m_dev->m_hasNetworkConnection = UbuntuDevice::NotAvailable;
                        emit featureDetected();
                        emit deviceNeedsSetup();
                    }
                    //detect other features
                    detectOpenSsh();
                }
            }
            break;
        }
        case UbuntuDevice::CloneNetwork:{
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NETWORK_CONF_COPIED));
            detectHasNetworkConnection();
            break;
        }
        case UbuntuDevice::DetectOpenSSH:{
            if (m_reply.trimmed() != QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NONE)
                    && m_reply.trimmed() != QLatin1String(Constants::EMPTY)) {

                if(m_dev->m_hasOpenSSHServer != UbuntuDevice::Available) {
                    m_dev->m_hasOpenSSHServer = UbuntuDevice::Available;
                    emit featureDetected();
                }

                ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceConnected);
                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_IS_INSTALLED).arg(m_reply.trimmed()));
                startSshService();
            } else {

                //device is only connected but not ready for use
                ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceConnected);

                if(m_dev->m_hasOpenSSHServer != UbuntuDevice::NotAvailable) {
                    m_dev->m_hasOpenSSHServer = UbuntuDevice::NotAvailable;
                    emit featureDetected();
                    emit deviceNeedsSetup();
                }

                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_NOT_INSTALLED));

                //detect other options
                detectDeviceWritableImage();
            }
            break;
        }
        case UbuntuDevice::InstallOpenSSH:{
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_INSTALLED));

            detectOpenSsh();
            break;
        }
        case UbuntuDevice::RemoveOpenSSH:{
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_REMOVED));

            detectOpenSsh();
            break;
        }
        case UbuntuDevice::StartOpenSSH:{

            if(m_dev->m_openSSHStarted != UbuntuDevice::Available) {
                m_dev->m_openSSHStarted = UbuntuDevice::Available;
                emit featureDetected();
            }
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_STARTED));

            enablePortForward();
            break;
        }
        case UbuntuDevice::EnablePortForwarding:{
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_PORTS_FORWARDED));

            deployPublicKey();
            break;
        }
        case UbuntuDevice::DeployPublicKey:{
            //ready to use
            ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceReadyToUse);
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_PUBLICKEY_AUTH_SET));

            detectDeviceWritableImage();
            break;
        }
        case UbuntuDevice::DetectDeviceWriteableImage:{
            if (m_reply.trimmed() == QLatin1String(Constants::ZERO_STR)) {
                m_dev->m_hasWriteableImage = UbuntuDevice::Available;
                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLEIMAGE));
            } else {
                m_dev->m_hasWriteableImage = UbuntuDevice::NotAvailable;
                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_READONLYIMAGE));
            }
            emit featureDetected();

            detectDeveloperTools();
            break;
        }
        case UbuntuDevice::DetectDeveloperTools:{
            setProcessState(UbuntuDevice::Done);
            stopProcess();

            if (m_reply.trimmed() == QLatin1String(Constants::ZERO_STR)) {
                m_dev->m_hasDeveloperTools = UbuntuDevice::NotAvailable;
                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_NOT_INSTALLED));
            } else {
                m_dev->m_hasDeveloperTools = UbuntuDevice::Available;
                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_INSTALLED));
            }
            emit featureDetected();
            break;
        }
        case UbuntuDevice::CloneTimeConfig:
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_TIME_CONF_COPIED));
            setProcessState(UbuntuDevice::Done);
            break;
        case UbuntuDevice::EnableRWImage:
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_ENABLED));
            detectDeviceWritableImage();
            break;
        case UbuntuDevice::DisableRWImage:
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_DISABLED));
            detectDeviceWritableImage();
            break;
        case UbuntuDevice::InstallDevTools:
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_WAS_INSTALLED));
            detectDeveloperTools();
            break;
        case UbuntuDevice::RemoveDevTools:
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_REMOVED));
            detectDeveloperTools();
            break;
        default:{
            break;
        }
    }
    m_reply.clear();
}

void UbuntuDeviceHelper::onMessage(const QString &msg) {
    m_reply.append(msg);

    QString message = msg;
    message.replace(QChar::fromLatin1('\n'),QLatin1String("<br/>"));

    addToLog(message);
}

void UbuntuDeviceHelper::onError(const QString &error)
{
    qDebug()<<"Received error "<<error;
    addToLog(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONERROR).arg(error));
}

void UbuntuDeviceHelper::detect()
{
    //start feature detection
    detectDeviceVersion();
}

void UbuntuDeviceHelper::detectOpenSsh()
{
    setProcessState(UbuntuDevice::DetectOpenSSH);

    m_dev->m_hasOpenSSHServer = UbuntuDevice::Unknown;
    emit featureDetected();

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTOPENSSH));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTOPENSSH_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::startSshService()
{
    setProcessState(UbuntuDevice::StartOpenSSH);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::enableDeveloperMode()
{
    setProcessState(UbuntuDevice::InstallOpenSSH);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::disableDeveloperMode()
{
    setProcessState(UbuntuDevice::RemoveOpenSSH);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_REMOVE));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_REMOVE_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::detectHasNetworkConnection()
{
    //@TODO use adb shell nmcli -t -f DEVICE,TYPE,STATE dev status
    //          adb shell nmcli dev list iface <intfc> | grep IP4.ADDRESS
    //      to find out the devices ip address
    setProcessState(UbuntuDevice::DetectNetworkConnection);

    m_dev->m_hasNetworkConnection = UbuntuDevice::Unknown;
    emit featureDetected();

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_HASNETWORK));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_HASNETWORK_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::detectDeviceVersion()
{
    setProcessState(UbuntuDevice::DetectDeviceVersion);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::detectDeviceWritableImage()
{
    setProcessState(UbuntuDevice::DetectDeviceWriteableImage);

    m_dev->m_hasWriteableImage = UbuntuDevice::Unknown;
    emit featureDetected();

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::detectDeveloperTools()
{
    setProcessState(UbuntuDevice::DetectDeveloperTools);

    m_dev->m_hasDeveloperTools = UbuntuDevice::Unknown;
    emit featureDetected();

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::deviceConnected()
{
    qDebug()<<"Device "<<m_dev->id().toString()<<" connected";
    ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceConnected);
    detect();
}

void UbuntuDeviceHelper::deviceDisconnected()
{
    qDebug()<<"Device "<<m_dev->id().toString()<<" disconnected";
    ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceDisconnected);

    setProcessState(UbuntuDevice::NotStarted);
    stopProcess();
    resetToDefaults();

    emit disconnected();
}

void UbuntuDeviceHelper::readProcessOutput(QProcess *proc)
{
    QString stderr = QString::fromLocal8Bit(proc->readAllStandardError());
    QString stdout = QString::fromLocal8Bit(proc->readAllStandardOutput());

    QString msg;
    if (!stderr.isEmpty()) {
        msg.append(stderr);
    }
    if (!stdout.isEmpty()) {
        msg.append(stdout);
    }

    if(!msg.isEmpty())
        onMessage(msg);
}

void UbuntuDeviceHelper::stopProcess()
{
    if(m_process) {
        m_process->disconnect(this);
        if(m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished();
        }
        m_process->deleteLater();
        m_process = 0;
    }
}

void UbuntuDeviceHelper::addToLog(const QString &msg)
{
    m_log.append(msg);
    emit message(msg);
}

void UbuntuDeviceHelper::setProcessState(const int newState)
{
    Q_ASSERT_X(newState >= UbuntuDevice::NotStarted && newState <= UbuntuDevice::Done,
               Q_FUNC_INFO,
               "State variable out of bounds");

    if( newState == m_dev->m_processState)
        return;

    if(m_dev->m_processState == UbuntuDevice::NotStarted || m_dev->m_processState == UbuntuDevice::Done)
        emit beginQueryDevice();
    else if((newState == UbuntuDevice::Done || newState == UbuntuDevice::NotStarted))
        emit endQueryDevice();

    m_dev->m_processState = static_cast<UbuntuDevice::ProcessState>(newState);
    emit detectionStateChanged();
}

void UbuntuDeviceHelper::resetToDefaults()
{
    m_clonedNwCount               = 0;
    m_reply.clear();
    m_dev->m_openSSHStarted       = UbuntuDevice::Unknown;
    m_dev->m_hasOpenSSHServer     = UbuntuDevice::Unknown;
    m_dev->m_hasNetworkConnection = UbuntuDevice::Unknown;
    m_dev->m_hasWriteableImage    = UbuntuDevice::Unknown;
    m_dev->m_hasDeveloperTools    = UbuntuDevice::Unknown;
    setProcessState(UbuntuDevice::NotStarted);

    emit featureDetected();
}

void UbuntuDeviceHelper::cloneTimeConfig()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::CloneTimeConfig);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONETIME));
    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONETIME_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::enableRWImage()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::EnableRWImage);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSWRITABLE));
    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSWRITABLE_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::disableRWImage()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::DisableRWImage);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSREADONLY));
    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSREADONLY_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

/*!
 * \brief UbuntuDeviceHelper::enablePortForward
 * Sets up the port forwarding for the device, this is executed
 * synchronously and will block the eventloop to make sure we only
 * use the same port once
 */
void UbuntuDeviceHelper::enablePortForward()
{
    setProcessState(UbuntuDevice::EnablePortForwarding);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_PORTFORWARD));

    stopProcess();

    if(!m_dev->m_localForwardedPorts.hasMore()) {
        m_dev->m_localForwardedPorts = UbuntuLocalPortsManager::getFreeRange(m_dev->id().toSetting().toString(),20);
        if(!m_dev->m_localForwardedPorts.hasMore()) {
            //Oh noes , no ports available

            endAction(tr("No ports available on the host, please detach some devices"));
            setProcessState(UbuntuDevice::Done);
            return;
        }
    }

    Utils::PortList copy = m_dev->m_localForwardedPorts;

    //first port is SSH port
    QSsh::SshConnectionParameters connParms = m_dev->sshParameters();
    connParms.port = copy.getNext();
    m_dev->setSshParameters(connParms);

    m_dev->setFreePorts(copy);

    QStringList ports;
    while(copy.hasMore())
        ports.append(QString::number(copy.getNext()));

    //@TODO per device settings
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    QString deviceSshPort = QString::number(connParms.port);

    QStringList args = QStringList()
            << m_dev->id().toSetting().toString()
            <<deviceSshPort
            <<ports;

    QProcess adb;
    adb.start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_PORTFORWARD_SCRIPT)
              .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH),args);

    adb.waitForFinished();
    readProcessOutput(&adb);

    processFinished(QString(),adb.exitCode());
}

void UbuntuDeviceHelper::installDevTools()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::InstallDevTools);

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT));
    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::removeDevTools()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::RemoveDevTools);

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT));
    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::deployPublicKey()
{
    setProcessState(UbuntuDevice::DeployPublicKey);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH));

    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));
    QString deviceUsername = settings.value(QLatin1String(Constants::SETTINGS_KEY_USERNAME),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_USERNAME)).toString();



    stopProcess();
    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString())
                 .arg(deviceUsername));
}

void UbuntuDeviceHelper::cloneNetwork()
{
    m_clonedNwCount++;

    setProcessState(UbuntuDevice::CloneNetwork);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK));

    stopProcess();
    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->id().toSetting().toString()));
}

void UbuntuDeviceHelper::startProcess(const QString &command)
{
    if(!m_process) {
        m_process = new QProcess(this);
        connect(m_process,SIGNAL(readyRead()),this,SLOT(onProcessReadyRead()));
        connect(m_process,SIGNAL(finished(int)),this,SLOT(onProcessFinished(int)));
        connect(m_process,SIGNAL(error(QProcess::ProcessError)),this,SLOT(onProcessError(QProcess::ProcessError)));
    }
    m_process->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_process->start(command);
}

void UbuntuDeviceHelper::onProcessReadyRead()
{
    readProcessOutput(m_process);
}

void UbuntuDeviceHelper::onProcessFinished(const int code)
{
    if (code != 0) {
        onError(QString::fromLocal8Bit(m_process->readAll()));
        processFinished(m_process->program(), code);
        return;
    }
    QString errorMsg = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (errorMsg.trimmed().length()>0) onError(errorMsg);
    QString msg = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (msg.trimmed().length()>0) onMessage(msg);

    processFinished(QString(),code);
}

void UbuntuDeviceHelper::onProcessError(const QProcess::ProcessError error)
{
    //if (m_process->exitCode() == 0) { return; }
    onError(QString(QLatin1String("ERROR: (%0) %1 %2")).arg(m_process->program()).arg(m_process->errorString()).arg(error));
}

void UbuntuDeviceHelper::beginAction(QString msg) {
    addToLog(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_BEGIN).arg(msg));
}

void UbuntuDeviceHelper::endAction(QString msg) {
    addToLog(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ACTION_END).arg(msg));
}

UbuntuDevice::UbuntuDevice()
    : LinuxDevice()
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
{
    setDeviceState(ProjectExplorer::IDevice::DeviceStateUnknown);
    loadDefaultConfig();
}

UbuntuDevice::UbuntuDevice(const QString &name, ProjectExplorer::IDevice::MachineType machineType, ProjectExplorer::IDevice::Origin origin, Core::Id id)
    : LinuxDevice(name,Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID),machineType,origin,id)
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
{
    setDeviceState(ProjectExplorer::IDevice::DeviceStateUnknown);
    loadDefaultConfig();
    setupPrivateKey();
    m_helper->init();
}

UbuntuDevice::UbuntuDevice(const UbuntuDevice &other)
    : LinuxDevice(other)
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
    , m_deviceInfoString(other.deviceInfoString())
{
    setDeviceState(ProjectExplorer::IDevice::DeviceDisconnected);
    setupPrivateKey();
    m_helper->init();
}

/*!
 * \brief UbuntuDevice::loadDefaultConfig
 * loads the default ssh connect and generic parameters from settings
 *
 * \note call this before fromMap() so every device can have its own settings
 */
void UbuntuDevice::loadDefaultConfig()
{
    QSettings settings(QLatin1String(Constants::SETTINGS_COMPANY),QLatin1String(Constants::SETTINGS_PRODUCT));
    settings.beginGroup(QLatin1String(Constants::SETTINGS_GROUP_DEVICE_CONNECTIVITY));

    QString ip            = settings.value(QLatin1String(Constants::SETTINGS_KEY_IP),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_IP)).toString();
    QString username      = settings.value(QLatin1String(Constants::SETTINGS_KEY_USERNAME),QLatin1String(Constants::SETTINGS_DEFAULT_DEVICE_USERNAME)).toString();
    //even though this is set here, it will be changed dynamically when the device is connected
    QString deviceSshPort = QString::number(Constants::SETTINGS_DEFAULT_DEVICE_SSH_PORT);
    QSsh::SshConnectionParameters params;
    params.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
    params.host = ip;
    params.port = deviceSshPort.toUInt();
    params.userName = username;
    params.timeout = 20;

    setSshParameters(params);
}

void UbuntuDevice::setupPrivateKey()
{
    if(!id().isValid())
        return;

    QSsh::SshConnectionParameters params = this->sshParameters();
    params.privateKeyFile = QString::fromLatin1(Constants::UBUNTU_DEVICE_SSHIDENTITY).arg(QDir::homePath()).arg(id().toSetting().toString());

    setSshParameters(params);
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

void UbuntuDevice::cloneNetwork()
{
    m_helper->cloneNetwork();
}

/*!
 * \brief UbuntuDevice::openTerminal
 * Opens a detached terminal, logged into the device
 */
void UbuntuDevice::openTerminal()
{
    QProcess p;

    QStringList args = QStringList()
            << id().toSetting().toString()
            << QString::number(sshParameters().port)
            << sshParameters().userName
            << sshParameters().host;

    p.startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSHCONNECT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                    , args
                    , QCoreApplication::applicationDirPath());

}

void UbuntuDevice::cloneTimeConfig()
{
    m_helper->cloneTimeConfig();
}

void UbuntuDevice::enablePortForward()
{
    m_helper->enablePortForward();
}

void UbuntuDevice::shutdown()
{
    QProcess p;
    QStringList args = QStringList()
            << id().toSetting().toString();

    //no need to redetect this should happen automagically
    p.startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SHUTDOWN_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                    , args
                    , QCoreApplication::applicationDirPath());
}

void UbuntuDevice::reboot()
{
    QProcess p;
    QStringList args = QStringList()
            << id().toSetting().toString();

    //no need to redetect this should happen automagically
    bool started = p.startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                                   , args
                                   , QCoreApplication::applicationDirPath());

    if(!started) {
        qDebug()<<"Could not start process "
               <<QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
              <<args
             <<p.errorString();
    }

}

void UbuntuDevice::rebootToRecovery()
{
    QProcess p;
    QStringList args = QStringList()
            << id().toSetting().toString();

    //no need to redetect this should happen automagically
    p.startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_RECOVERY_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                    , args
                    , QCoreApplication::applicationDirPath());
}

void UbuntuDevice::rebootToBootloader()
{
    QProcess p;
    QStringList args = QStringList()
            << id().toSetting().toString();

    //no need to redetect this should happen automagically
    p.startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_REBOOT_TO_BOOTLOADER_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                    , args
                    , QCoreApplication::applicationDirPath());
}

void UbuntuDevice::deployPublicKey()
{
    m_helper->deployPublicKey();
}

void UbuntuDevice::setDeveloperModeEnabled(const bool enabled)
{
    if(m_processState != Done)
        return;

    if(enabled && m_hasOpenSSHServer == NotAvailable)
        m_helper->enableDeveloperMode();
    else if(!enabled && m_hasOpenSSHServer == Available)
        m_helper->disableDeveloperMode();
}

UbuntuDevice::FeatureState UbuntuDevice::developerModeEnabled() const
{
    return m_hasOpenSSHServer;
}

void UbuntuDevice::setWriteableImageEnabled(const bool enabled)
{
    if(m_hasWriteableImage == Unknown)
        return;

    if(enabled && m_hasWriteableImage != Available)
        m_helper->enableRWImage();
    else if(!enabled && m_hasWriteableImage != NotAvailable)
        m_helper->disableRWImage();
}

void UbuntuDevice::setDeveloperToolsInstalled(const bool installed)
{
    if(installed && m_hasDeveloperTools != Available)
        m_helper->installDevTools();
    else if(!installed && m_hasDeveloperTools != NotAvailable)
        m_helper->removeDevTools();
}

void UbuntuDevice::setDeviceInfoString(const QString &info)
{
    m_deviceInfoString = info;
}

QString UbuntuDevice::deviceInfoString() const
{
    return m_deviceInfoString;
}

UbuntuDevice::FeatureState UbuntuDevice::hasNetworkConnection() const
{
    return m_hasNetworkConnection;
}

UbuntuDevice::FeatureState UbuntuDevice::hasWriteableImage() const
{
    return m_hasWriteableImage;
}

UbuntuDevice::FeatureState UbuntuDevice::hasDeveloperTools() const
{
    return m_hasDeveloperTools;
}

UbuntuDevice::ProcessState UbuntuDevice::detectionState() const
{
    return m_processState;
}

QString UbuntuDevice::detectionStateString( ) const
{
    switch (m_processState) {
        case NotStarted:
            return tr("");
        case DetectDeviceVersion:
            return tr("Detecting device version");
        case DetectNetworkConnection:
            return tr("Detecting network connection");
        case CloneNetwork:
            return tr("Cloning network configuration");
        case DetectOpenSSH:
            return tr("Detecting OpenSSH");
        case InstallOpenSSH:
            return tr("Installing OpenSSH");
        case RemoveOpenSSH:
            return tr("Removing OpenSSH");
        case StartOpenSSH:
            return tr("Starting OpenSSH");
        case EnablePortForwarding:
            return tr("Enable portforwarding");
        case DeployPublicKey:
            return tr("Deploying public key to device");
        case DetectDeviceWriteableImage:
            return tr("Detecting if image is writeable");
        case DetectDeveloperTools:
            return tr("Detecting if developer tools are installed");
        case FirstNonCriticalTask:
            return tr("");
        case CloneTimeConfig:
            return tr("Cloning time configuration");
        case EnableRWImage:
            return tr("Enabling writeable image");
        case DisableRWImage:
            return tr("Disabling writeable image");
        case InstallDevTools:
            return tr("Installing development tools");
        case RemoveDevTools:
            return tr("Removing development tools");
        case Done:
            return tr("Ready");
    }
    return QString();
}

ProjectExplorer::IDeviceWidget *UbuntuDevice::createWidget()
{
    return new RemoteLinux::GenericLinuxDeviceConfigurationWidget(sharedFromThis());
    //return new UbuntuDeviceConfigurationWidget(sharedFromThis());
}

QList<Core::Id> UbuntuDevice::actionIds() const
{
    return QList<Core::Id>();
}

QString UbuntuDevice::displayType() const
{
    return tr("Ubuntu Device");
}

ProjectExplorer::IDevice::Ptr  UbuntuDevice::clone() const
{
    return UbuntuDevice::Ptr(new UbuntuDevice(*this));
}

void UbuntuDevice::fromMap(const QVariantMap &map)
{
    LinuxDevice::fromMap(map);
    if(map.contains(DEVICE_INFO_KEY))
        m_deviceInfoString = map[DEVICE_INFO_KEY].toString();

    setupPrivateKey();
    m_helper->init();
}

QVariantMap UbuntuDevice::toMap() const
{
    QVariantMap map = LinuxDevice::toMap();
    map.insert(DEVICE_INFO_KEY,m_deviceInfoString);
    return map;
}

ProjectExplorer::DeviceProcess *UbuntuDevice::createProcess(QObject *parent) const
{
    return new UbuntuDeviceProcess(sharedFromThis(), parent);
}

UbuntuDevice::Ptr UbuntuDevice::sharedFromThis()
{
    return qSharedPointerCast<UbuntuDevice>(LinuxDevice::sharedFromThis());
}

UbuntuDevice::ConstPtr UbuntuDevice::sharedFromThis() const
{
    return qSharedPointerCast<const UbuntuDevice>(LinuxDevice::sharedFromThis());
}

UbuntuDevice::Ptr UbuntuDevice::create(const QString &name, const QString& serial, ProjectExplorer::IDevice::MachineType machineType, ProjectExplorer::IDevice::Origin origin)
{
    return Ptr(new UbuntuDevice(name,machineType,origin,Core::Id::fromSetting(serial)));
}

//////////////
/// UbuntuDeviceProcess
/////////////

static QString quote(const QString &s) { return Utils::QtcProcess::quoteArgUnix(s); }

UbuntuDeviceProcess::UbuntuDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device, QObject *parent)
    : RemoteLinux::LinuxDeviceProcess(device,parent)
{
}

void UbuntuDeviceProcess::setWorkingDirectory(const QString &directory)
{
    m_workingDir = directory;
}

void UbuntuDeviceProcess::terminate()
{
    LinuxDeviceProcess::terminate();
}

QString UbuntuDeviceProcess::fullCommandLine() const
{
    QStringList rcFiles = QStringList()
            << QLatin1String("/etc/profile")
            << QLatin1String("$HOME/.profile")
            << QLatin1String("$HOME/.bashrc");

    QStringList confFiles = QStringList()<<QString::fromLatin1("/etc/ubuntu-touch-session.d/$(getprop ro.product.device).conf");
    QString fullCommandLine = QString::fromLatin1("test -f %1 && for myenv in $(cat  %1); do `echo \"export\" $myenv`; done ;").arg(confFiles[0]);

    foreach (const QString &filePath, rcFiles)
        fullCommandLine += QString::fromLatin1("test -f %1 && . %1;").arg(filePath);
    if (!m_workingDir.isEmpty()) {
        fullCommandLine.append(QLatin1String("cd ")).append(quote(m_workingDir))
                .append(QLatin1String(" && "));
    }
    const QString envString = environment().toStringList().join(QLatin1String(" "));
    if (!envString.isEmpty())
        fullCommandLine.append(QLatin1Char(' ')).append(envString);
    if (!fullCommandLine.isEmpty())
        fullCommandLine += QLatin1Char(' ');
    fullCommandLine.append(quote(executable()));
    if (!arguments().isEmpty()) {
        fullCommandLine.append(QLatin1Char(' '));
        fullCommandLine.append(Utils::QtcProcess::joinArgsUnix(arguments()));
    }

    qDebug()<<fullCommandLine;
    return fullCommandLine;
}

} // namespace Internal
} // namespace Ubuntu
