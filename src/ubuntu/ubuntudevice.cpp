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
#include "ubuntudevicesignaloperation.h"
#include "ubuntuclicktool.h"
#include "localportsmanager.h"
#include "clicktoolchain.h"

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
#include <QRegularExpression>
#include <QSet>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

const QString DEVICE_SCALE_FACTOR(QStringLiteral("UbuntuDevice.EmulatorScaleFactor"));
const QString DEVICE_MEMORY_SETTING(QStringLiteral("UbuntuDevice.EmulatorMemory"));
const QString DEVICE_FRAMEWORK(QStringLiteral("UbuntuDevice.Framework"));
const QSet<QString> supportedScaleFactors {QStringLiteral("1.0"), QStringLiteral("0.9"), QStringLiteral("0.8"), QStringLiteral("0.7"),
            QStringLiteral("0.6"),QStringLiteral("0.5"), QStringLiteral("0.4"), QStringLiteral("0.3"), QStringLiteral("0.2"),
            QStringLiteral("0.1")};
const QSet<QString> supportedMemorySettings {QStringLiteral("512"), QStringLiteral("768"), QStringLiteral("1024")};

/*!
 * \class UbuntuDeviceHelper
 * Helper class that collects informations about the
 * device, and can be used to monitor connect/disconnect
 * and other device events
 */
UbuntuDeviceHelper::UbuntuDeviceHelper(UbuntuDevice *dev)
    : QObject(0)
    , m_clonedNwCount(0)
    , m_errorCount(0)
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
    if(!m_deviceWatcher) {
        QString watchKey;
        if(m_dev->machineType() == ProjectExplorer::IDevice::Emulator) {
            if (!m_dev->imageName().isEmpty()) {
                m_deviceWatcher = new UbuntuEmulatorNotifier(this);
                watchKey = m_dev->imageName();
            }
        } else {
            if (!m_dev->serialNumber().isEmpty()) {
                m_deviceWatcher = new UbuntuDeviceNotifier(this);
                watchKey = m_dev->serialNumber();
            }
        }

        if (m_deviceWatcher) {
            m_deviceWatcher->stopMonitoring();
            m_deviceWatcher->startMonitoring(watchKey);
            connect(m_deviceWatcher,SIGNAL(deviceConnected()),this,SLOT(deviceConnected()));
            connect(m_deviceWatcher,SIGNAL(deviceDisconnected()),this,SLOT(deviceDisconnected()));
        }
    }

    if(m_deviceWatcher && m_deviceWatcher->isConnected()) {
        deviceConnected();
    }
}

void UbuntuDeviceHelper::waitForEmulatorStart()
{
    setProcessState(UbuntuDevice::WaitForEmulatorStart);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_WAIT_FOR_EMULATOR_MESSAGE));

    stopProcess();
    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_WAIT_FOR_EMULATOR_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->imageName()));
}

void UbuntuDeviceHelper::waitForBoot()
{
    //at this point the serial ID should be known
    m_dev->setupPrivateKey();

    setProcessState(UbuntuDevice::WaitForBootAdbAccess);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_WAIT_FOR_BOOT_MESSAGE));

    stopProcess();
    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_WAIT_FOR_BOOT_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::processFinished(const QString &, const int code)
{
    Q_UNUSED(code)


    if(m_errorCount > 3) {
        setProcessState(UbuntuDevice::Failed);
        m_reply.clear();
        return;
    }

    switch(m_dev->m_processState) {
        case UbuntuDevice::WaitForEmulatorStart: {

            if(code != 0) {
                m_errorCount++;
                waitForEmulatorStart();
                break;
            } else {
                m_errorCount = 0;
            }

            m_dev->m_emulatorSerial = m_reply.trimmed();
            emit deviceInfoUpdated();

            waitForBoot();
            break;
        }
        case UbuntuDevice::WaitForBoot:
        case UbuntuDevice::WaitForBootAdbAccess:
        case UbuntuDevice::WaitForBootDeviceLock: {
            if(code != 0) {
                m_errorCount++;
                waitForBoot();
                break;
            } else {
                m_errorCount = 0;
            }
            //for now the script will wait forever until the shell is available
            detect();
            break;
        }
        case UbuntuDevice::DetectDeviceVersion:{
            if(code != 0) {
                m_errorCount++;
                detect();
                break;
            }

            QStringList options = m_reply.split(QStringLiteral("\n"),QString::SkipEmptyParts);
            if(debug) qDebug()<<options;

            if(options.length() < 6) {
                m_errorCount++;
                detect();
                break;
            }

            QString framework    = options[5].trimmed();
            if(!UbuntuClickFrameworkProvider::getSupportedFrameworks().contains(framework)) {
                addToLog(tr("Device detection reported unknown framework %1").arg(framework));
                m_errorCount++;
                detect();
                break;
            }

            m_errorCount = 0;

            m_dev->m_framework = framework;

            //will trigger the device updated signal
            m_dev->setDeviceInfo(options[1].trimmed(),
                    options[0].trimmed(),
                    options[2].trimmed());

            detectHasNetworkConnection();
            break;
        }
        case UbuntuDevice::DetectNetworkConnection:{
            if(code != 0) {
                m_errorCount++;
                detectHasNetworkConnection();
                break;
            } else {
                m_errorCount = 0;
            }

            if (m_reply.trimmed() == QString::fromLatin1(Constants::ONE_STR)) {
                // we have network
                m_clonedNwCount = 0;
                if(m_dev->m_hasNetworkConnection != UbuntuDevice::Available) {
                    m_dev->m_hasNetworkConnection = UbuntuDevice::Available;
                }
                emit featureDetected();
                detectOpenSsh();

            } else {
                if(m_dev->m_hasNetworkConnection != UbuntuDevice::NotAvailable) {
                    m_dev->m_hasNetworkConnection = UbuntuDevice::NotAvailable;
                }
                emit featureDetected();
                emit deviceNeedsSetup();
                //detect other features
                detectOpenSsh();
            }
            break;
        }
        case UbuntuDevice::CloneNetwork:{
            m_errorCount = 0;
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NETWORK_CONF_COPIED));
            detectHasNetworkConnection();
            break;
        }
        case UbuntuDevice::DetectOpenSSH:{
            if(code != 0) {
                m_errorCount++;
                detectOpenSsh();
                break;
            } else {
                m_errorCount = 0;
            }

            if (m_reply.trimmed() != QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_NONE)
                    && m_reply.trimmed() != QLatin1String(Constants::EMPTY)) {

                if(m_dev->m_hasOpenSSHServer != UbuntuDevice::Available) {
                    m_dev->m_hasOpenSSHServer = UbuntuDevice::Available;
                }
                emit featureDetected();

                ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceConnected);
                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_IS_INSTALLED).arg(m_reply.trimmed()));
                startSshService();
            } else {

                //device is only connected but not ready for use
                ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceConnected);

                if(m_dev->m_hasOpenSSHServer != UbuntuDevice::NotAvailable) {
                    m_dev->m_hasOpenSSHServer = UbuntuDevice::NotAvailable;
                }
                emit featureDetected();
                emit deviceNeedsSetup();

                endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_NOT_INSTALLED));

                //detect other options
                detectDeviceWritableImage();
            }
            break;
        }
        case UbuntuDevice::InstallOpenSSH:{
            m_errorCount = 0;
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_INSTALLED));

            detectOpenSsh();
            break;
        }
        case UbuntuDevice::RemoveOpenSSH:{
            m_errorCount = 0;
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_REMOVED));

            detectOpenSsh();
            break;
        }
        case UbuntuDevice::StartOpenSSH:{
            if(code != 0) {
                m_errorCount++;
                startSshService();
                break;
            } else {
                m_errorCount = 0;
            }

            if(m_dev->m_openSSHStarted != UbuntuDevice::Available) {
                m_dev->m_openSSHStarted = UbuntuDevice::Available;
                emit featureDetected();
            }
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_STARTED));

            enablePortForward();
            break;
        }
        case UbuntuDevice::EnablePortForwarding:{
            m_errorCount = 0;
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_PORTS_FORWARDED));

            deployPublicKey();
            break;
        }
        case UbuntuDevice::DeployPublicKey:{
            if(code != 0) {
                m_errorCount++;
                deployPublicKey();
                break;
            } else {
                m_errorCount = 0;
            }
            //ready to use
            ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceReadyToUse);
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_PUBLICKEY_AUTH_SET));

            detectDeviceWritableImage();
            break;
        }
        case UbuntuDevice::DetectDeviceWriteableImage:{
            if(code != 0) {
                m_errorCount++;
                detectDeviceWritableImage();
                break;
            } else {
                m_errorCount = 0;
            }

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
            if(code != 0) {
                m_errorCount++;
                detectDeveloperTools();
                break;
            } else {
                m_errorCount = 0;
            }

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
        case UbuntuDevice::EnableRWImage:
            m_errorCount = 0;
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_ENABLED));
            detectDeviceWritableImage();
            break;
        case UbuntuDevice::DisableRWImage:
            m_errorCount = 0;
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_DISABLED));
            detectDeviceWritableImage();
            break;
        case UbuntuDevice::InstallDevTools:
            m_errorCount = 0;
            endAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_WAS_INSTALLED));
            detectDeveloperTools();
            break;
        case UbuntuDevice::RemoveDevTools:
            m_errorCount = 0;
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

    if(m_dev->m_processState == UbuntuDevice::WaitForBootAdbAccess) {
        if(m_reply.contains(QStringLiteral("DevLocked"))) {
            setProcessState(UbuntuDevice::WaitForBootDeviceLock);
        }
        else if(m_reply.contains(QStringLiteral("DevUnLocked"))) {
            setProcessState(UbuntuDevice::WaitForBoot);
        }
    }
}

void UbuntuDeviceHelper::onError(const QString &error)
{
    addToLog(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ONERROR).arg(error));
}

void UbuntuDeviceHelper::onStdOut(const QString &stdout)
{
    QString message = stdout;
    message.replace(QChar::fromLatin1('\n'),QLatin1String("<br/>"));
    addToLog(message);
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
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::startSshService()
{
    setProcessState(UbuntuDevice::StartOpenSSH);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_STARTSSHSERVICE_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::enableDeveloperMode()
{
    setProcessState(UbuntuDevice::InstallOpenSSH);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL));

    m_dev->m_hasOpenSSHServer = UbuntuDevice::Unknown;
    emit featureDetected();

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_INSTALL_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::disableDeveloperMode()
{
    setProcessState(UbuntuDevice::RemoveOpenSSH);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_REMOVE));

    m_dev->m_hasOpenSSHServer = UbuntuDevice::Unknown;
    emit featureDetected();

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSH_REMOVE_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
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
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::detectDeviceVersion()
{
    setProcessState(UbuntuDevice::DetectDeviceVersion);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION));

    stopProcess();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
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
                 .arg(m_dev->serialNumber()));
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
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::deviceConnected()
{
    if(debug) qDebug()<<"Device "<<m_dev->id().toString()<<" connected";

    ProjectExplorer::DeviceManager::instance()->setDeviceState(m_dev->id(),ProjectExplorer::IDevice::DeviceConnected);

    if(m_dev->machineType() == ProjectExplorer::IDevice::Emulator)
        waitForEmulatorStart();
    else
        waitForBoot();
}

void UbuntuDeviceHelper::deviceDisconnected()
{
    if(debug) qDebug()<<"Device "<<m_dev->id().toString()<<" disconnected";
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
        onError(stderr);
    }
    if (!stdout.isEmpty()) {
        msg.append(stdout);
        onStdOut(stdout);
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
    Q_ASSERT_X(newState >= UbuntuDevice::NotStarted && newState <= UbuntuDevice::MaxState,
               Q_FUNC_INFO,
               "State variable out of bounds");

    if( newState == m_dev->m_processState)
        return;

    if(m_dev->m_processState == UbuntuDevice::NotStarted || m_dev->m_processState == UbuntuDevice::Done || m_dev->m_processState == UbuntuDevice::Failed)
        emit beginQueryDevice();
    else if((newState == UbuntuDevice::Done || newState == UbuntuDevice::NotStarted || newState == UbuntuDevice::Failed))
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

void UbuntuDeviceHelper::enableRWImage()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::EnableRWImage);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSWRITABLE));
    stopProcess();

    m_dev->m_hasWriteableImage = UbuntuDevice::Unknown;
    emit featureDetected();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSWRITABLE_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::disableRWImage()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::DisableRWImage);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSREADONLY));
    stopProcess();

    m_dev->m_hasWriteableImage = UbuntuDevice::Unknown;
    emit featureDetected();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_MAKEFSREADONLY_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
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

    /* always query for a free range! When detaching and reattaching device1
     * while plugging in device2 inbetween, it could happen that we reuse
     * ports that were assigned to device2 in the meantime
     * See bug LP:1396406
     */
    m_dev->m_localForwardedPorts = UbuntuLocalPortsManager::getFreeRange(m_dev->serialNumber(),10);
    if(!m_dev->m_localForwardedPorts.hasMore()) {
        //Oh noes , no ports available

        endAction(tr("No ports available on the host, please detach some devices"));
        setProcessState(UbuntuDevice::Failed);
        return;
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
            << m_dev->serialNumber()
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

    m_dev->m_hasDeveloperTools = UbuntuDevice::Unknown;
    emit featureDetected();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::removeDevTools()
{
    if(m_dev->m_processState < UbuntuDevice::FirstNonCriticalTask && m_dev->m_processState != UbuntuDevice::NotStarted)
        return;

    setProcessState(UbuntuDevice::RemoveDevTools);

    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT));
    stopProcess();

    m_dev->m_hasDeveloperTools = UbuntuDevice::Unknown;
    emit featureDetected();

    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
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
                 .arg(m_dev->serialNumber())
                 .arg(deviceUsername));
}

void UbuntuDeviceHelper::cloneNetwork()
{
    m_clonedNwCount++;

    setProcessState(UbuntuDevice::CloneNetwork);
    beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK));

    m_dev->m_hasNetworkConnection = UbuntuDevice::Unknown;
    emit featureDetected();

    stopProcess();
    startProcess(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_CLONENETWORK_SCRIPT)
                 .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                 .arg(m_dev->serialNumber()));
}

void UbuntuDeviceHelper::startProcess(const QString &command)
{
    if(debug) qDebug()<<"Starting process: "<<command;

    if(!m_process) {
        m_process = new QProcess(this);
        connect(m_process,SIGNAL(readyRead()),this,SLOT(onProcessReadyRead()));
        connect(m_process,SIGNAL(finished(int)),this,SLOT(onProcessFinished(int)));
        connect(m_process,SIGNAL(error(QProcess::ProcessError)),this,SLOT(onProcessError(QProcess::ProcessError)));
        connect(m_process,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(onProcessStateChanged(QProcess::ProcessState)));
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
    QString errorMsg = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (errorMsg.trimmed().length()>0) onError(errorMsg);
    QString msg = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (msg.trimmed().length()>0) onMessage(msg);

    processFinished(m_process->program(),code);
}

void UbuntuDeviceHelper::onProcessError(const QProcess::ProcessError error)
{
    QString errorString = QStringLiteral("ERROR: (%0) %1 %2").arg(m_process->program()).arg(m_process->errorString()).arg(error);

    if(debug) qDebug()<<"Received Process Error: "<<error<<errorString;

    onError(errorString);
}

void UbuntuDeviceHelper::onProcessStateChanged(QProcess::ProcessState newState)
{
    if(!debug)
        return;

    switch(newState) {
        case QProcess::NotRunning: {
            if(debug) qDebug()<<QStringLiteral("Process not running");
            break;
        }
        case QProcess::Starting: {
            if(debug) qDebug()<<QStringLiteral("Process starting");
            break;
        }
        case QProcess::Running: {
            if(debug) qDebug()<<QStringLiteral("Process running");
            break;
        }
    }
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

UbuntuDevice::UbuntuDevice(const QString &name, ProjectExplorer::IDevice::MachineType machineType, ProjectExplorer::IDevice::Origin origin, Core::Id id, const QString &architecture)
    : LinuxDevice(name,Core::Id(Constants::UBUNTU_DEVICE_TYPE_ID).withSuffix(architecture),machineType,origin,id)
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
    , m_scaleFactor(QStringLiteral("1.0"))
    , m_memory(QStringLiteral("512"))
{
    setDeviceState(ProjectExplorer::IDevice::DeviceStateUnknown);
    loadDefaultConfig();
    m_helper->init();
}

UbuntuDevice::UbuntuDevice(const UbuntuDevice &other)
    : LinuxDevice(other)
    , m_helper(new UbuntuDeviceHelper(this))
    , m_processState(NotStarted)
    , m_deviceInfo(other.m_deviceInfo)
    , m_modelInfo(other.m_modelInfo)
    , m_productInfo(other.m_productInfo)
    , m_ubuntuVersion(other.m_ubuntuVersion)
    , m_deviceVersion(other.m_deviceVersion)
    , m_imageVersion(other.m_imageVersion)
    , m_scaleFactor(other.m_scaleFactor)
    , m_memory(other.m_memory)
{
    setDeviceState(ProjectExplorer::IDevice::DeviceDisconnected);
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
    params.privateKeyFile = QString::fromLatin1(Constants::UBUNTU_DEVICE_SSHIDENTITY).arg(QDir::homePath());

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
    if(machineType() == Emulator)
        return m_emulatorSerial;

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
            << serialNumber()
            << QString::number(sshParameters().port)
            << sshParameters().userName
            << sshParameters().host;

    p.startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_SSHCONNECT_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                    , args
                    , QCoreApplication::applicationDirPath());

}

void UbuntuDevice::enablePortForward()
{
    m_helper->enablePortForward();
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

void UbuntuDevice::setDeviceInfo(const QString &productInfo, const QString &modelInfo, const QString &deviceInfo)
{
    m_productInfo = productInfo;
    m_modelInfo = modelInfo;
    m_deviceInfo = deviceInfo;
    emit m_helper->deviceInfoUpdated();
}

QString UbuntuDevice::modelInfo() const
{
    return m_modelInfo;
}

QString UbuntuDevice::deviceInfo() const
{
    return m_deviceInfo;
}

QString UbuntuDevice::productInfo() const
{
    return m_productInfo;
}

void UbuntuDevice::setEmulatorInfo(const QString &ubuntuVersion, const QString &deviceVersion, const QString &imageVersion)
{
    m_ubuntuVersion = ubuntuVersion;
    m_deviceVersion = deviceVersion;
    m_imageVersion  = imageVersion;
    emit m_helper->deviceInfoUpdated();
}

QString UbuntuDevice::ubuntuVersion() const
{
    return m_ubuntuVersion;
}

QString UbuntuDevice::deviceVersion() const
{
    return m_deviceVersion;
}

QString UbuntuDevice::imageVersion() const
{
    return m_imageVersion;
}

bool UbuntuDevice::startEmulator()
{
    if ( machineType() != IDevice::Emulator )
        return false;

    QStringList args = QStringList() << imageName() << m_memory << m_scaleFactor;
    return QProcess::startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                            ,args
                            ,QCoreApplication::applicationDirPath());
}


QString UbuntuDevice::imageName() const
{
    return id().toSetting().toString();
}

QString UbuntuDevice::architecture() const
{
    QString arch = this->type().suffixAfter(Constants::UBUNTU_DEVICE_TYPE_ID);
    if(debug) qDebug()<<"Reporting device architecture as: "<<arch;
    return arch;
}

QString UbuntuDevice::framework() const
{
    return m_framework;
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
        case WaitForEmulatorStart:
            return tr("Waiting for the emulator to start up");
        case WaitForBoot:
            return tr("Waiting for the device to finish booting");
        case WaitForBootAdbAccess:
            return tr("Waiting for adb access, make sure the developer mode is enabled");
        case WaitForBootDeviceLock:
            return tr("Waiting for the device, make sure it is unlocked");
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
        case EnableRWImage:
            return tr("Enabling writeable image");
        case DisableRWImage:
            return tr("Disabling writeable image");
        case InstallDevTools:
            return tr("Installing development tools");
        case RemoveDevTools:
            return tr("Removing development tools");
        case Done: {
            if(deviceState() == ProjectExplorer::IDevice::DeviceReadyToUse)
                return tr("Ready to use");
            else
                return tr("Connected but not ready");
        }
        case Failed:
            return tr("Detection failed");
    }
    return QString();
}

QString UbuntuDevice::scaleFactor() const
{
    return m_scaleFactor;
}

bool UbuntuDevice::setScaleFactor(const QString &factor)
{
    if(supportedScaleFactors.contains(factor)) {
        m_scaleFactor = factor;
        emit m_helper->deviceInfoUpdated();
        return true;
    }
    return false;
}

QString UbuntuDevice::memorySetting() const
{
    return m_memory;
}

bool UbuntuDevice::setMemorySetting(const QString &memory)
{
    if(supportedMemorySettings.contains(memory)) {
        m_memory = memory;
        emit m_helper->deviceInfoUpdated();
        return true;
    }
    return false;
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
    return tr("Ubuntu Device (%1)").arg(architecture());
}

ProjectExplorer::IDevice::Ptr  UbuntuDevice::clone() const
{
    return UbuntuDevice::Ptr(new UbuntuDevice(*this));
}

void UbuntuDevice::fromMap(const QVariantMap &map)
{
    LinuxDevice::fromMap(map);

    if (map.contains(DEVICE_SCALE_FACTOR)) {
        m_scaleFactor = map[DEVICE_SCALE_FACTOR].toString();
        if(!supportedScaleFactors.contains(m_scaleFactor))
            m_scaleFactor = QStringLiteral("1.0");
    }

    if (map.contains(DEVICE_MEMORY_SETTING)) {
        m_memory = map[DEVICE_MEMORY_SETTING].toString();

        if(!supportedMemorySettings.contains(m_memory))
            m_memory = QStringLiteral("512");
    }

    if (map.contains(DEVICE_FRAMEWORK)) {
        m_framework = map[DEVICE_FRAMEWORK].toString();

        //if a invalid framework is in the device, let it redetect
        if(!UbuntuClickFrameworkProvider::getSupportedFrameworks().contains(m_framework))
            m_framework.clear();
    }

    m_helper->init();
}

QVariantMap UbuntuDevice::toMap() const
{
    QVariantMap map = LinuxDevice::toMap();
    map.insert(DEVICE_SCALE_FACTOR, m_scaleFactor);
    map.insert(DEVICE_MEMORY_SETTING, m_memory);
    map.insert(DEVICE_FRAMEWORK, m_framework);
    return map;
}

ProjectExplorer::DeviceProcess *UbuntuDevice::createProcess(QObject *parent) const
{
    return new UbuntuDeviceProcess(sharedFromThis(), parent);
}

ProjectExplorer::DeviceProcessSignalOperation::Ptr UbuntuDevice::signalOperation() const
{
    UbuntuDeviceSignalOperation::Ptr p(new UbuntuDeviceSignalOperation(sharedFromThis()));
    return p;
}

UbuntuDevice::Ptr UbuntuDevice::sharedFromThis()
{
    return qSharedPointerCast<UbuntuDevice>(LinuxDevice::sharedFromThis());
}

UbuntuDevice::ConstPtr UbuntuDevice::sharedFromThis() const
{
    return qSharedPointerCast<const UbuntuDevice>(LinuxDevice::sharedFromThis());
}

UbuntuDevice::Ptr UbuntuDevice::create(const QString &name, const QString& serial, ProjectExplorer::IDevice::MachineType machineType, const QString &archName, ProjectExplorer::IDevice::Origin origin)
{
    return Ptr(new UbuntuDevice(name,machineType,origin,Core::Id::fromSetting(serial),archName));
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
    //return QStringLiteral("%1 %2").arg(quote(executable()),Utils::QtcProcess::joinArgsUnix(arguments()));

    QStringList rcFiles = QStringList()
            << QLatin1String("/etc/profile")
            << QLatin1String("$HOME/.profile")
            << QLatin1String("$HOME/.bashrc");

    QString fullCommandLine;
    foreach (const QString &filePath, rcFiles)
        fullCommandLine += QString::fromLatin1("test -f %1 && . %1;").arg(filePath);
    if (!m_workingDir.isEmpty()) {
        fullCommandLine.append(QLatin1String("cd ")).append(quote(m_workingDir))
                .append(QLatin1String(" && "));
    }

    fullCommandLine.append(quote(executable()));
    if (!arguments().isEmpty()) {
        fullCommandLine.append(QLatin1Char(' '));
        fullCommandLine.append(Utils::QtcProcess::joinArgs(arguments(),Utils::OsTypeLinux));
    }

    if(debug) qDebug()<<fullCommandLine;

    return fullCommandLine;
}

} // namespace Internal
} // namespace Ubuntu
