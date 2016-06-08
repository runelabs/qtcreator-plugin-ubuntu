#ifndef UBUNTUWAITFORDEVICEDIALOG_H
#define UBUNTUWAITFORDEVICEDIALOG_H

#include <QProgressDialog>

#include "ubuntudevice.h"

#include <coreplugin/id.h>


namespace Ubuntu{
namespace Internal{

class UbuntuWaitForDeviceDialog : public QProgressDialog
{
    Q_OBJECT
public:
    explicit UbuntuWaitForDeviceDialog(QWidget *parent = 0);
    void show (UbuntuDevice::ConstPtr device);

signals:
    void deviceReady ();

protected slots:
    void handleDeviceUpdated ();
    void handleCanceled ();
    void updateLabelText ();

private:
    UbuntuDevice::ConstPtr m_dev;


};

}
}

#endif // UBUNTUWAITFORDEVICEDIALOG_H
