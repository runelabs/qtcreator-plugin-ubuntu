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

#ifndef UBUNTU_INTERNAL_CLICKTOOLCHAIN_H
#define UBUNTU_INTERNAL_CLICKTOOLCHAIN_H

#include <projectexplorer/gcctoolchain.h>
#include "ubuntuclicktool.h"

namespace Ubuntu {
namespace Internal {

class ClickToolChainFactory;

class ClickToolChain : public ProjectExplorer::GccToolChain
{
    friend class ClickToolChainFactory;

    // ToolChain interface
public:
    ClickToolChain(const UbuntuClickTool::Target &target,Detection d);

    virtual QList<Utils::FileName> suggestedMkspecList() const;
    virtual Utils::FileName suggestedDebugger() const;
    virtual QString type() const;
    virtual QString typeDisplayName() const;
    virtual bool isValid() const;
    virtual void addToEnvironment(Utils::Environment &env) const;
    virtual QString makeCommand(const Utils::Environment &) const;
    virtual bool operator ==(const ProjectExplorer::ToolChain &tc) const;
    virtual ProjectExplorer::ToolChainConfigWidget *configurationWidget();
    virtual QVariantMap toMap() const;

    QString gnutriplet () const;
    const UbuntuClickTool::Target &clickTarget () const;

protected:
    virtual bool fromMap(const QVariantMap &data);

    ClickToolChain(const ClickToolChain& other);
    ClickToolChain();

private:
    UbuntuClickTool::Target m_clickTarget;

    // ToolChain interface
public:
    virtual Utils::FileName compilerCommand() const;
};

class ClickToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT
public:
    ClickToolChainFactory();

    // ToolChainFactory interface
public:
    virtual QList<ProjectExplorer::ToolChain *> autoDetect();
    virtual bool canRestore(const QVariantMap &data);
    virtual ProjectExplorer::ToolChain *restore(const QVariantMap &data);

    static QList<ProjectExplorer::ToolChain *> createToolChainsForClickTargets();
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CLICKTOOLCHAIN_H
