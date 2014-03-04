#ifndef UBUNTU_INTERNAL_UBUNTUDEVICECONFIGURATIONWIDGET_H
#define UBUNTU_INTERNAL_UBUNTUDEVICECONFIGURATIONWIDGET_H

#include <projectexplorer/devicesupport/idevicewidget.h>
#include "ubuntudevice.h"

namespace Ubuntu {
namespace Internal {

namespace Ui {
class UbuntuDeviceConfigurationWidget;
}

class UbuntuDeviceConfigurationWidget : public ProjectExplorer::IDeviceWidget
{
    Q_OBJECT

public:
    explicit UbuntuDeviceConfigurationWidget(const ProjectExplorer::IDevice::Ptr &deviceConfig, QWidget *parent = 0);
    ~UbuntuDeviceConfigurationWidget();

    // IDeviceWidget interface
    virtual void updateDeviceFromUi();

private slots:
    void updateWidgets();
    void on_message (const QString &message);
    void on_pushButtonSshInstall_clicked();
    void on_pushButtonSshRemove_clicked();
    void on_pushButtonCloneTimeConfig_clicked();
    void on_pushButtonPortForward_clicked();
    void on_pushButtonSshSetupPublicKey_clicked();
    void on_pushButtonSshConnect_clicked();
    void on_pushButton_filesystem_rw_disable_clicked();
    void on_pushButton_filesystem_rw_enable_clicked();
    void on_pushButtonPlatformDevelopmentRemove_clicked();
    void on_pushButtonPlatformDevelopment_clicked();
    void on_pushButtonReboot_clicked();
    void on_pushButtonShutdown_clicked();
    void on_pushButtonRebootToBootloader_clicked();
    void on_pushButtonRebootToRecovery_clicked();

private:
    Ui::UbuntuDeviceConfigurationWidget *ui;
    UbuntuDevice::Ptr m_ubuntuDev;
};


} // namespace Internal
} // namespace Ubuntu
#endif // UBUNTU_INTERNAL_UBUNTUDEVICECONFIGURATIONWIDGET_H
