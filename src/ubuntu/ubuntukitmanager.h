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
#include <ubuntu/device/remote/ubuntudevice.h>
#include <projectexplorer/kit.h>

namespace Debugger{
class DebuggerItem;
}

namespace CMakeProjectManager{
class CMakeTool;
}

namespace Ubuntu {
namespace Internal {

class UbuntuQtVersion;

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
    static UbuntuQtVersion *createOrFindQtVersion(ClickToolChain* tc);
    static CMakeProjectManager::CMakeTool *createOrFindCMakeTool(ClickToolChain* tc);
    static CMakeProjectManager::CMakeTool *createCMakeTool(ClickToolChain *tc);
    static CMakeProjectManager::CMakeTool *createCMakeTool(const UbuntuClickTool::Target &target);
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUKITMANAGER_H
