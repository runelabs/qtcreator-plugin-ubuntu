#ifndef UBUNTU_INTERNAL_CONTAINERDEVICE_H
#define UBUNTU_INTERNAL_CONTAINERDEVICE_H

#include <remotelinux/linuxdevice.h>


namespace Ubuntu {
namespace Internal {

class ContainerDevicePrivate;
class ContainerDevice : public RemoteLinux::LinuxDevice
{
public:
    typedef QSharedPointer<ContainerDevice> Ptr;
    typedef QSharedPointer<const ContainerDevice> ConstPtr;

    static Ptr create(Core::Id type, Core::Id id);

    ~ContainerDevice ();

    static Core::Id createIdForContainer(const QString &name);
    QString containerName() const;

    // IDevice interface
    virtual ProjectExplorer::IDeviceWidget *createWidget() override;
    virtual QList<Core::Id> actionIds() const override;
    virtual void executeAction(Core::Id actionId, QWidget *parent) override;
    virtual IDevice::Ptr clone() const override;
    virtual QString displayNameForActionId(Core::Id actionId) const override;
    virtual QString displayType() const override;
    virtual ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const override;

protected:
    ContainerDevice(Core::Id type, Core::Id id);
    ContainerDevice(const ContainerDevice &other);

private:
    Q_DECLARE_PRIVATE(ContainerDevice)
    ContainerDevicePrivate *d_ptr;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CONTAINERDEVICE_H
