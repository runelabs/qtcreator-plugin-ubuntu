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

#ifndef UBUNTU_INTERNAL_LOCALPORTSMANAGER_H
#define UBUNTU_INTERNAL_LOCALPORTSMANAGER_H

#include <QObject>
#include <QIODevice>
#include <utils/portlist.h>

namespace Ubuntu {
namespace Internal {

class UbuntuLocalPortsManager : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuLocalPortsManager();
    ~UbuntuLocalPortsManager();
    static void setPortsRange (const int first, const int last);
    static Utils::PortList getFreeRange ( const QString &serial, const int count);
    static Utils::PortList getFreeRange ( const QString &serial, const int count, QIODevice *in );

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
