#ifndef UBUNTU_INTERNAL_UBUNTUDEVICE_H
#define UBUNTU_INTERNAL_UBUNTUDEVICE_H

#include <remotelinux/linuxdevice.h>
#include <coreplugin/id.h>

class UbuntuDeviceNotifier;

namespace Ubuntu {
namespace Internal {

class UbuntuProcess;
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

signals:
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
    void detectHasNetworkConnection();
    void detectDeviceVersion();
    void detectDeviceWritableImage();
    void detectDeveloperTools();
    void beginAction(QString msg);
    void endAction(QString msg);
    void resetToDefaults();

protected slots:
    void onMessage(QString msg);
    void processFinished ( const QString&, const int code);
    void deviceConnected ();
    void deviceDisconnected ();

private:
    QString m_reply;
    UbuntuDevice  *m_dev;
    UbuntuProcess *m_process;
    UbuntuDeviceNotifier *m_deviceWatcher;
};

class UbuntuDevice : public RemoteLinux::LinuxDevice
{
    Q_DECLARE_TR_FUNCTIONS(Ubuntu::Internal::UbuntuDevice)
    friend class UbuntuDeviceHelper;
    friend class UbuntuDeviceFactory;

public:
    enum FeatureState {
        NotAvailable,
        Unknown,
        Available
    };

    enum ProcessState {
        NotStarted,
        DetectDeviceVersion,
        DetectNetworkConnection,
        DetectOpenSSH,
        InstallOpenSSH,
        StartOpenSSH,
        EnablePortForwarding,
        DeployPublicKey,
        DetectDeviceWriteableImage,
        DetectDeveloperTools,
        Done
    };

    typedef QSharedPointer<UbuntuDevice> Ptr;
    typedef QSharedPointer<const UbuntuDevice> ConstPtr;

    virtual ~UbuntuDevice ();

    static Ptr create();
    static Ptr create(const QString &name,const QString &serial, MachineType machineType, Origin origin = ManuallyAdded);

    QString serialNumber () const;
    UbuntuDeviceHelper *helper () const;

    bool    developerModeEnabled ();
    void    enableDeveloperMode  ();


protected:
    UbuntuDevice();
    UbuntuDevice(const QString &name,MachineType machineType, Origin origin, Core::Id id);
    UbuntuDevice(const UbuntuDevice &other);
    void loadDefaultSSHParams();
private:
    UbuntuDeviceHelper* m_helper;
    FeatureState    m_openSSHStarted;
    FeatureState    m_hasOpenSSHServer;
    FeatureState    m_hasNetworkConnection;
    FeatureState    m_hasWriteableImage;
    FeatureState    m_hasDeveloperTools;
    ProcessState    m_processState;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEVICE_H
