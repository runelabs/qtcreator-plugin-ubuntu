#ifndef UBUNTU_INTERNAL_UBUNTUDIRECTUPLOADSTEP_H
#define UBUNTU_INTERNAL_UBUNTUDIRECTUPLOADSTEP_H

#include <remotelinux/abstractremotelinuxdeploystep.h>
#include <QPointer>

namespace RemoteLinux { class GenericDirectUploadService; }

namespace Ubuntu {
namespace Internal {

class UbuntuWaitForDeviceDialog;

class UbuntuDirectUploadStep : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_OBJECT

public:
    UbuntuDirectUploadStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuDirectUploadStep(ProjectExplorer::BuildStepList *bsl, UbuntuDirectUploadStep *other);
    ~UbuntuDirectUploadStep();

    // BuildStep interface
    virtual void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool initInternal(QString *error = 0) override;

    RemoteLinux::AbstractRemoteLinuxDeployService *deployService() const override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    static Core::Id stepId();
    static QString displayName();

private slots:
    void projectNameChanged();
    void handleWaitDialogCanceled();
    void handleDeviceReady ();

private:
    RemoteLinux::GenericDirectUploadService *m_deployService;
    bool m_foundClickPackage;

    //wait support for the device/emulator to come up
    QPointer<UbuntuWaitForDeviceDialog> m_waitDialog;
    QFutureInterface<bool> *m_future;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDIRECTUPLOADSTEP_H
