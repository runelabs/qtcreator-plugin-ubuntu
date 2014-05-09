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
#ifndef UBUNTU_INTERNAL_UBUNTUKITMANAGER_H
#define UBUNTU_INTERNAL_UBUNTUKITMANAGER_H

#include "clicktoolchain.h"
#include "ubuntudevice.h"
#include <projectexplorer/kit.h>

namespace Debugger{
class DebuggerItem;
}

namespace Ubuntu {
namespace Internal {

class UbuntuKitManager : public QObject
{
    Q_OBJECT
public:
    UbuntuKitManager();

    static void autoCreateKit  ( UbuntuDevice::Ptr device );
    static void autoDetectKits ();
    static ProjectExplorer::Kit *createKit (ClickToolChain* tc);
    static QVariant createOrFindDebugger(const Utils::FileName &path);
    static void fixKit (ProjectExplorer::Kit* k);
    static QList<ClickToolChain *> clickToolChains();
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUKITMANAGER_H