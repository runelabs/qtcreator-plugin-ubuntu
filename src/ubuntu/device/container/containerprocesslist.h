#ifndef UBUNTU_INTERNAL_CONTAINERPROCESSLIST_H
#define UBUNTU_INTERNAL_CONTAINERPROCESSLIST_H

#include "containerdevice.h"
#include <projectexplorer/devicesupport/deviceprocesslist.h>


namespace Ubuntu {
namespace Internal {

class ContainerProcessList : public ProjectExplorer::DeviceProcessList
{
    Q_OBJECT

public:
    ContainerProcessList(const ContainerDevice::ConstPtr &device, QObject *parent = 0);

    static QList<ProjectExplorer::DeviceProcessItem> getContainerProcesses(const QString &container);

private:
    void doUpdate();
    void doKillProcess(const ProjectExplorer::DeviceProcessItem &process);

private slots:
    void handleUpdate();
    void reportDelayedKillStatus(const QString &errorMessage);

private:
    ProjectExplorer::DeviceProcessSignalOperation::Ptr m_sig;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CONTAINERPROCESSLIST_H
