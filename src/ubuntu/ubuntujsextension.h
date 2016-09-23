/*
 * Copyright 2016 Canonical Ltd.
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
#ifndef UBUNTUJSEXTENSION_H
#define UBUNTUJSEXTENSION_H

#include <QObject>

namespace Ubuntu {
namespace Internal {

class UbuntuJsExtension : public QObject
{
    Q_OBJECT
public:
    explicit UbuntuJsExtension(QObject *parent = 0);

    Q_INVOKABLE QString developerId() const;
};

}}

#endif // UBUNTUJSEXTENSION_H
