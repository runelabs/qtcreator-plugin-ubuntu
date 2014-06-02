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
#ifndef UBUNTUCMAKETOOL_H
#define UBUNTUCMAKETOOL_H

#include <cmakeprojectmanager/cmaketool.h>
#include <projectexplorer/kit.h>

namespace Ubuntu {
namespace Internal {

class UbuntuCMakeTool : public CMakeProjectManager::CMakeTool
{
    Q_OBJECT
public:
    explicit UbuntuCMakeTool(QObject *parent = 0);
    explicit UbuntuCMakeTool(const Core::Id &id, QObject *parent = 0);

    // ICMakeTool interface
    virtual void addToEnvironment(Utils::Environment &env) const  override;
    virtual QString displayName () override;
    virtual bool isValid() const override;

    void setEnvironment (const Utils::Environment &env);

private:
    bool m_hasEnvironment;
    Utils::Environment m_env;

};

class UbuntuCMakeToolFactory : public CMakeProjectManager::ICMakeToolFactory
{
    Q_OBJECT
public:
    virtual bool canCreate (const Core::Id& id) const override;
    virtual CMakeProjectManager::ICMakeTool *create (const Core::Id& id) override;
};

} //namespace Internal
} //namespace Ubuntu

#endif // UBUNTUCMAKETOOL_H
