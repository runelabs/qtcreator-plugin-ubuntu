/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */
#include "localportsmanager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

UbuntuLocalPortsManager* UbuntuLocalPortsManager::m_instance = 0;

UbuntuLocalPortsManager::UbuntuLocalPortsManager( ) :
    QObject(0),
    m_first(10000),
    m_last (10029) //there are max 30 ports available, more would break adb
{
    Q_ASSERT_X(m_instance == 0, Q_FUNC_INFO,"There can be only one instance of UbuntuLocalPortsManager");
    m_instance = this;
}

UbuntuLocalPortsManager::~UbuntuLocalPortsManager()
{
    if(m_instance == this)
        m_instance = 0;
}

void UbuntuLocalPortsManager::setPortsRange(const int first, const int last)
{
    m_instance->m_first = first;
    m_instance->m_last  = last;
}

/*!
 * \brief UbuntuLocalPortsManager::getFreeRange
 * Queries adb which ports are already in use and returns a list
 * of ports available.
 * \bug As long as there is only once instance of QtC running this should
 * be no problem, but when a second one is started it may result in a race condition
 * and ports could get reassigned and leave one QtC with a wrong portlist
 */
Utils::PortList UbuntuLocalPortsManager::getFreeRange(const QString &serial, const int count)
{
    QProcess adb;
    adb.start(QLatin1String("adb")
              ,QStringList()<<QLatin1String("forward")
              <<QLatin1String("--list"));

    adb.waitForFinished();

    if (adb.exitCode() != 0) {
        if(debug) qDebug()<<"Adb failed to run "<<adb.errorString();
        return Utils::PortList();
    }

    adb.setReadChannel(QProcess::StandardOutput);
    return getFreeRange(serial,count,&adb);
}

Utils::PortList UbuntuLocalPortsManager::getFreeRange( const QString &serial, const int count, QIODevice *in )
{
    if(!in)
        return Utils::PortList();

    QString str = QString::fromLocal8Bit(in->readAll());

    QRegularExpression regExpPorts(QLatin1String("^\\s*(\\S+)\\s*tcp:([0-9]+)\\s*tcp:([0-9]+)"),QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matchIter = regExpPorts.globalMatch(str);

    Utils::PortList freePorts;
    Utils::PortList usedPorts;
    while(matchIter.hasNext()) {
        QRegularExpressionMatch match = matchIter.next();

        QString device      = match.captured(1);
        int localPort       = match.captured(2).toInt();
        QString remotePort  = match.captured(3);

        //if a port is already forwarded to the device, we can just reuse it
        if (serial == device && localPort >= m_instance->m_first && localPort <= m_instance->m_last) {
            if(debug) qDebug()<<"Found port already linked to device: "<<localPort<<":"<<remotePort;

            freePorts.addPort(Utils::Port(localPort));

            if (freePorts.count() == count)
                break;
        } else {
            if(debug) qDebug()<<"Found port in use: "<<localPort<<":"<<remotePort;
            usedPorts.addPort(Utils::Port(localPort));
        }
    }

    int required = count - freePorts.count();
    int firstPort = m_instance->m_first;
    for(int i = 0, found = 0; found < required && firstPort+i < m_instance->m_last; i++) {
        int port = firstPort + i;

        //is the port in use?
        if(usedPorts.contains(Utils::Port(port)))
            continue;

        //is that port already assigned to us?
        if(freePorts.contains(Utils::Port(port)))
            continue;

        freePorts.addPort(Utils::Port(port));
        found++;
        if(debug) qDebug()<<"Found free port: "<<port;
    }

    return freePorts;
}
} // namespace Internal
} // namespace Ubuntu
