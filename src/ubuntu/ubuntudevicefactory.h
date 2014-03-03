#ifndef UBUNTU_INTERNAL_UBUNTUDEVICEFACTORY_H
#define UBUNTU_INTERNAL_UBUNTUDEVICEFACTORY_H

#include <projectexplorer/devicesupport/idevicefactory.h>

namespace Ubuntu {
namespace Internal {

class UbuntuDeviceFactory : public ProjectExplorer::IDeviceFactory
{
    Q_OBJECT
public:
    UbuntuDeviceFactory(QObject* parent = 0);

    // IDeviceFactory interface
    virtual QString displayNameForId(Core::Id type) const;
    virtual QList<Core::Id> availableCreationIds() const;
    virtual bool canCreate() const;
    virtual ProjectExplorer::IDevice::Ptr create(Core::Id id) const;
    virtual bool canRestore(const QVariantMap &map) const;
    virtual ProjectExplorer::IDevice::Ptr restore(const QVariantMap &map) const;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEVICEFACTORY_H
