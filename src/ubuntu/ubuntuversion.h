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

#ifndef UBUNTUVERSION_H
#define UBUNTUVERSION_H

#include <QObject>
#include <coreplugin/featureprovider.h>
#include "ubuntuconstants.h"

namespace Ubuntu {
namespace Internal {


class UbuntuVersion : public QObject
{
    Q_OBJECT

public:
    explicit UbuntuVersion();
    
public slots:
    QString id() { return m_id; }
    QString release() { return m_release; }
    QString codename() { return m_codename; }
    QString description() { return m_description; }

    Core::FeatureSet features();

protected:
    QString m_id;
    QString m_release;
    QString m_codename;
    QString m_description;
};
}
}

#endif // UBUNTUVERSION_H
