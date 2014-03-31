#include "ubuntuemulatornotifier.h"

#include <QTimer>
#include <QRegularExpression>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuEmulatorNotifier
 * Polls adb if a specific
 */

UbuntuEmulatorNotifier::UbuntuEmulatorNotifier(QObject *parent) :
    IUbuntuDeviceNotifier(parent)
{
    m_pollTimout  = new QTimer(this);
    m_pollProcess = new QProcess(this);

    connect(m_pollTimout,SIGNAL(timeout()),this,SLOT(pollTimeout()));
    connect(m_pollProcess,SIGNAL(readyRead()),this,SLOT(pollProcessReadyRead()));
    connect(m_pollProcess,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(pollProcessFinished(int,QProcess::ExitStatus)));
}

void UbuntuEmulatorNotifier::startMonitoring(const QString &serialNumber)
{
    m_serial = serialNumber;
    m_pollTimout->setInterval(5000);
    m_pollTimout->start();
}

void UbuntuEmulatorNotifier::stopMonitoring()
{
    m_pollTimout->stop();
    m_pollProcess->kill();
}

bool UbuntuEmulatorNotifier::isConnected() const
{
    return (m_lastState == QLatin1String("device"));
}

void UbuntuEmulatorNotifier::pollTimeout()
{
    m_buffer.clear();
    m_pollProcess->start(QLatin1String("adb"),
                         QStringList()
                             <<QLatin1String("-s")
                             <<m_serial
                             <<QLatin1String("get-state"));
}

void UbuntuEmulatorNotifier::pollProcessReadyRead()
{
    m_buffer.append(m_pollProcess->readAllStandardOutput());
}

void UbuntuEmulatorNotifier::pollProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        QString str = QString::fromLocal8Bit(m_buffer.data());

        QRegularExpression expr(QLatin1String("(device|unknown|offline|bootloader)"));
        QRegularExpressionMatch match = expr.match(str);
        if(match.hasMatch()) {
            bool wasConnected = isConnected();
            QString newState = match.captured(1);
            if(newState != m_lastState) {
                m_lastState = newState;
                if(isConnected() && !wasConnected)
                    emit deviceConnected();
            }
        }

    }
}

} // namespace Internal
} // namespace Ubuntu
