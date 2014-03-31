#ifndef UBUNTU_INTERNAL_LOCALPORTSMANAGER_H
#define UBUNTU_INTERNAL_LOCALPORTSMANAGER_H

#include <QObject>
#include <utils/portlist.h>

namespace Ubuntu {
namespace Internal {

class UbuntuLocalPortsManager : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuLocalPortsManager();
    static void setPortsRange (const int first, const int last);
    static Utils::PortList getFreeRange ( const QString &serial, const int count);

signals:

public slots:

private:
    static UbuntuLocalPortsManager *m_instance;
    int m_first;
    int m_last;
    Utils::PortList m_usedPorts;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_LOCALPORTSMANAGER_H
