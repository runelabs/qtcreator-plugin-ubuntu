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

    virtual QList<Utils::FileName> suggestedMkspecList() const override;
    virtual Utils::FileName suggestedDebugger() const override;
    virtual QString typeDisplayName() const override;
    virtual bool isValid() const override;
    virtual void addToEnvironment(Utils::Environment &env) const override;
    virtual QString makeCommand(const Utils::Environment &) const override;
    virtual bool operator ==(const ProjectExplorer::ToolChain &tc) const override;
    virtual ProjectExplorer::ToolChainConfigWidget *configurationWidget() override;
    virtual QVariantMap toMap() const override;

    QString gnutriplet () const;
    static QString gnutriplet (const ProjectExplorer::Abi &abi);
    const UbuntuClickTool::Target &clickTarget () const;

    static ProjectExplorer::Abi architectureNameToAbi ( const QString &arch );
    static QList<QString> supportedArchitectures ();

protected:
    virtual bool fromMap(const QVariantMap &data) override;

    ClickToolChain(const ClickToolChain& other);
    ClickToolChain();

private:
    UbuntuClickTool::Target m_clickTarget;

    // ToolChain interface
public:
    virtual Utils::FileName compilerCommand() const override;
};

class ClickToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT
public:
    ClickToolChainFactory();

    // ToolChainFactory interface
public:
    virtual QList<ProjectExplorer::ToolChain *> autoDetect(const QList<ProjectExplorer::ToolChain *> &alreadyKnown) override;
    virtual bool canRestore(const QVariantMap &data) override;
    virtual ProjectExplorer::ToolChain *restore(const QVariantMap &data) override;

    static QList<ProjectExplorer::ToolChain *> createToolChainsForClickTargets(const QList<ProjectExplorer::ToolChain *> &alreadyKnown);
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_CLICKTOOLCHAIN_H
