/*
 * Copyright 2013 Canonical Ltd.
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
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#ifndef UBUNTUAPIMODE_H
#define UBUNTUAPIMODE_H

#include <QObject>

#include "ubuntuwebmode.h"

namespace ProjectExplorer { class Project; }

namespace Ubuntu {
namespace Internal {

class UbuntuAPIMode : public UbuntuWebMode
{
    Q_OBJECT

public:
    explicit UbuntuAPIMode(QObject *parent = 0);

protected slots:
    // UbuntuWebMode interface
    virtual void modeChanged(Core::IMode *mode);
    void startupProjectChanged (ProjectExplorer::Project *startup);
    void activeTargetChanged ();

private:
    QUrl defaultUrl () const;
    QMetaObject::Connection m_currProjectConnection;
};

}
}

#endif // UBUNTUAPIMODE_H
