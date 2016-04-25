#ifndef CONTAINERDEVICE_P_H
#define CONTAINERDEVICE_P_H

#include <QObject>
#include <QProcess>

namespace Ubuntu {
namespace Internal {

class ContainerDevice;
class ContainerDevicePrivate : public QObject
{
    Q_OBJECT

public:
    ContainerDevicePrivate (Ubuntu::Internal::ContainerDevice *q);
    void resetProcess();
    QString userName () const;
    void reset ();

    void showWarningMessage (const QString &msg);

public slots:
    void handleDetectionStepFinished();

public:

    enum State {
        Initial,
        GetStatus,
        DeployKey,
        Finished
    } m_deviceState = Initial;
    QString m_deviceIP;

private:
    void printProcessError ();

private:
    ContainerDevice *q_ptr;
    QProcess *m_detectionProcess;
    Q_DECLARE_PUBLIC(ContainerDevice)
};


}}


#endif // CONTAINERDEVICE_P_H

