#ifndef UBUNTU_INTERNAL_CONTAINERDEVICEFACTORY_H
#define UBUNTU_INTERNAL_CONTAINERDEVICEFACTORY_H

#include <projectexplorer/devicesupport/idevicefactory.h>

namespace Ubuntu {
namespace Internal {

class ContainerDeviceFactory : public ProjectExplorer::IDeviceFactory
{
    Q_OBJECT
public:
    ContainerDeviceFactory(QObject *parent = 0);

    // IDeviceFactory interface
public:
    virtual QString displayNameForId(Core::Id type) const override;
    virtual QList<Core::Id> availableCreationIds() const override;
    virtual ProjectExplorer::IDevice::Ptr create(Core::Id id) const override;
    virtual bool canRestore(const QVariantMap &map) const override;
    virtual ProjectExplorer::IDevice::Ptr restore(const QVariantMap &map) const override;
    virtual bool canCreate() const override;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CONTAINERDEVICEFACTORY_H
