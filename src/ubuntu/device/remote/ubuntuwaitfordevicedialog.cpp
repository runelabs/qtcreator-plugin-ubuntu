#include "ubuntuwaitfordevicedialog.h"

#include <projectexplorer/devicesupport/devicemanager.h>

namespace Ubuntu{
namespace Internal{

UbuntuWaitForDeviceDialog::UbuntuWaitForDeviceDialog(QWidget *parent) :
    QProgressDialog(parent)
{
    connect(this,SIGNAL(canceled()),this,SLOT(handleCanceled()));
}

void UbuntuWaitForDeviceDialog::show(UbuntuDevice::ConstPtr device)
{
    m_dev = device;
    connect(ProjectExplorer::DeviceManager::instance(),SIGNAL(deviceUpdated(Core::Id)),
            this,SLOT(handleDeviceUpdated()));

    setMinimum(0);
    setMaximum(0);
    this->open();

    handleDeviceUpdated();
}

void UbuntuWaitForDeviceDialog::handleDeviceUpdated()
{
    if(!m_dev) {
        cancel();
        return;
    }

    if(m_dev->deviceState() == ProjectExplorer::IDevice::DeviceReadyToUse) {
        ProjectExplorer::DeviceManager::instance()->disconnect(this);
        accept();
        emit deviceReady();
        return;
    }

    updateLabelText();

}

void UbuntuWaitForDeviceDialog::handleCanceled()
{
    ProjectExplorer::DeviceManager::instance()->disconnect(this);
}

void UbuntuWaitForDeviceDialog::updateLabelText()
{
    bool isEmulator = m_dev->machineType() == ProjectExplorer::IDevice::Emulator;

    switch (m_dev->deviceState()) {
        case ProjectExplorer::IDevice::DeviceDisconnected:
        case ProjectExplorer::IDevice::DeviceStateUnknown:
            setLabelText(isEmulator ? tr("Please start your emulator: %1").arg(m_dev->displayName())
                                    : tr("Please attach your device: %1").arg(m_dev->displayName()));
            break;
        case ProjectExplorer::IDevice::DeviceConnected:
            setLabelText(tr("Waiting for your %1 to get ready").arg(isEmulator ? tr("emulator") : tr("device")));
            break;
        case ProjectExplorer::IDevice::DeviceReadyToUse:
            setLabelText(tr("Finished"));
            break;
    }
}


}
}


