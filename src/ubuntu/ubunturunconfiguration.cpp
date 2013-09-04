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

#include "ubunturunconfiguration.h"

using namespace Ubuntu::Internal;

ProjectExplorer::Abi UbuntuRunConfiguration::Qabi() const {
    ProjectExplorer::Abi hostAbi = ProjectExplorer::Abi::hostAbi();
    return ProjectExplorer::Abi(hostAbi.architecture(), hostAbi.os(), hostAbi.osFlavor(),
                                ProjectExplorer::Abi::RuntimeQmlFormat, hostAbi.wordWidth());
}
