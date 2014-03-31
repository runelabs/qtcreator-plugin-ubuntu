#ifndef UBUNTU_INTERNAL_UBUNTUDEVICE_H
#define UBUNTU_INTERNAL_UBUNTUDEVICE_H

#include <remotelinux/linuxdevice.h>
#include <remotelinux/linuxdeviceprocess.h>
#include <coreplugin/id.h>
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
    void connected        ();
    void disconnected     ();
    void deviceNeedsSetup ();
    void featureDetected  ();
    void message (const QString &message);

protected:
    void init   ();
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
    void startProcess    (const QString& command);

protected slots:
    void onProcessReadyRead ();
    void onProcessFinished  (const int code);
    void onProcessError     (const QProcess::ProcessError error);
    void onMessage(const QString &msg);
    void onError(const QString &error);
    void processFinished (const QString&, const int code);
    void deviceConnected ();
    void deviceDisconnected ();

protected:
    void stopProcess ();
    void addToLog (const QString &msg);
    void setProcessState (const int newState);

private:
    QString m_log;
    QString m_reply;
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
        DetectDeviceVersion,
        DetectNetworkConnection,
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
        Done
    };

    typedef QSharedPointer<UbuntuDevice> Ptr;
    typedef QSharedPointer<const UbuntuDevice> ConstPtr;

    virtual ~UbuntuDevice ();

    static Ptr create();
    static Ptr create(const QString &name,const QString &serial, MachineType machineType, Origin origin = ManuallyAdded);

    QString serialNumber () const;
    UbuntuDeviceHelper *helper () const;

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

    void setDeviceInfoString (const QString &info);
    QString deviceInfoString () const;

    FeatureState developerModeEnabled () const;
    FeatureState hasNetworkConnection () const;
    FeatureState hasWriteableImage    () const;
    FeatureState hasDeveloperTools    () const;

    ProcessState detectionState () const;

    // IDevice interface
    virtual ProjectExplorer::IDeviceWidget *createWidget();
    virtual QList<Core::Id> actionIds() const;
    virtual QString displayType() const;
    virtual ProjectExplorer::IDevice::Ptr clone() const;
    virtual void fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;
    virtual ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const;

    Ptr sharedFromThis ();
    ConstPtr sharedFromThis() const;
protected:
    UbuntuDevice();
    UbuntuDevice(const QString &name,MachineType machineType, Origin origin, Core::Id id);
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
    QString         m_deviceInfoString;
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

    // DeviceProcess interface
public:
    virtual void terminate();
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEVICE_H
