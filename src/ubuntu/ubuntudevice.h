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
#ifndef UBUNTU_INTERNAL_UBUNTUDEVICE_H
#define UBUNTU_INTERNAL_UBUNTUDEVICE_H

#include <remotelinux/linuxdevice.h>
#include <remotelinux/linuxdeviceprocess.h>
#include <coreplugin/id.h>
#include <projectexplorer/abi.h>
#include <utils/portlist.h>
#include <QProcess>

class IUbuntuDeviceNotifier;

namespace Ubuntu {
namespace Internal {

class UbuntuDevice;
class UbuntuDeviceFactory;

class UbuntuDeviceHelper : public QObject
{
    Q_OBJECT
    friend class UbuntuDevice;
public:
    explicit UbuntuDeviceHelper(UbuntuDevice* dev);
    ~UbuntuDeviceHelper();

    UbuntuDevice *device () const;
    QString log () const;
    void refresh ();

signals:
    void beginQueryDevice ();
    void endQueryDevice   ();
    void detectionStateChanged ();
    void connected        ();
    void disconnected     ();
    void deviceNeedsSetup ();
    void featureDetected  ();
    void deviceInfoUpdated ();
    void message (const QString &message);

protected:
    void init   ();
    void waitForEmulatorStart();
    void waitForBoot ();
    void detect ();
    void detectOpenSsh();
    void startSshService();
    void enableDeveloperMode();
    void disableDeveloperMode();
    void detectHasNetworkConnection();
    void detectDeviceVersion();
    void detectDeviceWritableImage();
    void detectDeveloperTools();
    void beginAction(QString msg);
    void endAction(QString msg);
    void resetToDefaults();
    void cloneTimeConfig ();
    void enableRWImage ();
    void disableRWImage ();
    void enablePortForward ();
    void installDevTools ();
    void removeDevTools  ();
    void deployPublicKey ();
    void cloneNetwork ();
    void startProcess    (const QString& command);

protected slots:
    void onProcessReadyRead ();
    void onProcessFinished  (const int code);
    void onProcessError     (const QProcess::ProcessError error);
    void onProcessStateChanged (QProcess::ProcessState newState);
    void onMessage(const QString &msg);
    void onError(const QString &error);
    void onStdOut(const QString &stdout);
    void processFinished (const QString&, const int code);
    void deviceConnected ();
    void deviceDisconnected ();

protected:
    void readProcessOutput (QProcess* proc);
    void stopProcess ();
    void addToLog (const QString &msg);
    void setProcessState (const int newState);

private:
    QString m_log;
    QString m_reply;
    int     m_clonedNwCount;
    int     m_errorCount;
    UbuntuDevice  *m_dev;
    QProcess *m_process;
    IUbuntuDeviceNotifier *m_deviceWatcher;
};

class UbuntuDevice : public RemoteLinux::LinuxDevice
{
    Q_DECLARE_TR_FUNCTIONS(Ubuntu::Internal::UbuntuDevice)
    friend class UbuntuDeviceHelper;
    friend class UbuntuDeviceFactory;

public:
    enum FeatureState {
        NotAvailable, //Feature is not available on the device
        Unknown,      //Feature is not detected yet
        Available     //Feature is available on the device
    };

    enum ProcessState {
        NotStarted,
        WaitForEmulatorStart,
        WaitForBootAdbAccess,
        WaitForBootDeviceLock,
        WaitForBoot,
        DetectDeviceVersion,
        DetectNetworkConnection,
        CloneNetwork,
        DetectOpenSSH,
        InstallOpenSSH,
        RemoveOpenSSH,
        StartOpenSSH,
        EnablePortForwarding,
        DeployPublicKey,
        DetectDeviceWriteableImage,
        DetectDeveloperTools,
        FirstNonCriticalTask,
        CloneTimeConfig,
        EnableRWImage,
        DisableRWImage,
        InstallDevTools,
        RemoveDevTools,
        Done,
        Failed,
        MaxState = Failed
    };

    typedef QSharedPointer<UbuntuDevice> Ptr;
    typedef QSharedPointer<const UbuntuDevice> ConstPtr;

    virtual ~UbuntuDevice ();

    static Ptr create();
    static Ptr create(const QString &name,const QString &serial, MachineType machineType, const QString &archName, Origin origin = ManuallyAdded);

    QString serialNumber () const;
    UbuntuDeviceHelper *helper () const;

    void cloneNetwork    ();
    void openTerminal    ();
    void cloneTimeConfig ();
    void enablePortForward ();
    void shutdown ();
    void reboot   ();
    void rebootToRecovery   ();
    void rebootToBootloader ();
    void deployPublicKey    ();
    void setDeveloperModeEnabled     ( const bool enabled = true );
    void setWriteableImageEnabled    ( const bool enabled = true );
    void setDeveloperToolsInstalled  ( const bool installed = true );

    void setDeviceInfo   (const QString &productInfo, const QString &modelInfo, const QString &deviceInfo);
    QString modelInfo() const;
    QString deviceInfo() const;
    QString productInfo() const;
    QString imageName() const;
    QString architecture() const;
    QString framework() const;

    void setEmulatorInfo (const QString &ubuntuVersion, const QString &deviceVersion, const QString &imageVersion);
    QString ubuntuVersion() const;
    QString deviceVersion() const;
    QString imageVersion() const;
    bool startEmulator ();

    FeatureState developerModeEnabled () const;
    FeatureState hasNetworkConnection () const;
    FeatureState hasWriteableImage    () const;
    FeatureState hasDeveloperTools    () const;

    ProcessState detectionState () const;
    QString detectionStateString () const;

    QString scaleFactor () const;
    bool setScaleFactor (const QString &factor);

    QString memorySetting () const;
    bool setMemorySetting (const QString &memory);

    // IDevice interface
    virtual ProjectExplorer::IDeviceWidget *createWidget() override;
    virtual QList<Core::Id> actionIds() const override;
    virtual QString displayType() const override;
    virtual ProjectExplorer::IDevice::Ptr clone() const override;
    virtual void fromMap(const QVariantMap &map) override;
    virtual QVariantMap toMap() const override;
    virtual ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const override;
    virtual ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;

    Ptr sharedFromThis ();
    ConstPtr sharedFromThis() const;
protected:
    UbuntuDevice();
    UbuntuDevice(const QString &name,MachineType machineType, Origin origin, Core::Id id, const QString &architecture);
    UbuntuDevice(const UbuntuDevice &other);
    void loadDefaultConfig();
    void setupPrivateKey  ();
private:
    UbuntuDeviceHelper* m_helper;
    FeatureState    m_openSSHStarted;
    FeatureState    m_hasOpenSSHServer;
    FeatureState    m_hasNetworkConnection;
    FeatureState    m_hasWriteableImage;
    FeatureState    m_hasDeveloperTools;
    ProcessState    m_processState;
    QString         m_deviceInfo;
    QString         m_modelInfo;
    QString         m_productInfo;
    QString         m_emulatorSerial;
    QString         m_ubuntuVersion;
    QString         m_deviceVersion;
    QString         m_imageVersion;
    QString         m_scaleFactor;
    QString         m_memory;
    QString         m_framework;
    Utils::PortList m_localForwardedPorts;

private:
    UbuntuDevice &operator=(const UbuntuDevice &);
};

class UbuntuDeviceProcess : public RemoteLinux::LinuxDeviceProcess
{
    Q_OBJECT

public:
    explicit UbuntuDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device,
                                QObject *parent = 0);

    void setWorkingDirectory(const QString &directory) override;

    // DeviceProcess interface
public:
    virtual void terminate() override;

private:
    // SshDeviceProcess interface
    virtual QString fullCommandLine() const override;
    QString m_workingDir;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEVICE_H
