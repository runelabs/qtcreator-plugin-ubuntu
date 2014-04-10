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
#include "ubuntucmaketool.h"
#include "ubuntuconstants.h"
#include <QUuid>

namespace Ubuntu {
namespace Internal {

/*!
 * \brief createId
 * Adds a suffix to every tool id, so every kit has its
 * own cmake instance
 */
static Core::Id createId()
{
    Core::Id newId(Constants::UBUNTU_CLICK_CMAKE_TOOL_ID);
    return newId.withSuffix(QUuid::createUuid().toString());
}

UbuntuCMakeTool::UbuntuCMakeTool(QObject *parent)
    : CMakeProjectManager::CMakeTool(parent)
    , m_hasEnvironment(false)
{
    setId(createId());
}

UbuntuCMakeTool::UbuntuCMakeTool(const Core::Id &id, QObject *parent)
    : CMakeProjectManager::CMakeTool(parent)
    , m_hasEnvironment(false)
{
    setId(id);
}

QString UbuntuCMakeTool::displayName()
{
    return tr("Ubuntu Chroot CMake");
}

void UbuntuCMakeTool::addToEnvironment(Utils::Environment &env) const
{
    Utils::Environment::const_iterator i = m_env.constBegin();
    for(;i != m_env.constEnd();i++)
        env.set(i.key(),i.value());
}

bool UbuntuCMakeTool::isValid() const
{
    if(!m_hasEnvironment)
        return false;
    return CMakeTool::isValid();
}

void UbuntuCMakeTool::setEnvironment(const Utils::Environment &env)
{
    m_hasEnvironment = true;
    m_env = env;
    setCMakeExecutable(QString::fromLatin1(Constants::UBUNTU_CLICK_CMAKE_WRAPPER).arg(Constants::UBUNTU_SCRIPTPATH));
}

bool UbuntuCMakeToolFactory::canCreate(const Core::Id &id) const
{
    return (id.toString().startsWith(QLatin1String(Constants::UBUNTU_CLICK_CMAKE_TOOL_ID)));
}

CMakeProjectManager::ICMakeTool *UbuntuCMakeToolFactory::create(const Core::Id &id)
{
    if(!canCreate(id))
        return 0;

    return new UbuntuCMakeTool(id);
}

} //namespace Internal
} //namespace Ubuntu
