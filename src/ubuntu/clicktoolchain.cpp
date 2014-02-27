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

#include "clicktoolchain.h"
#include "ubuntuconstants.h"

#include <utils/fileutils.h>
#include <projectexplorer/abi.h>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>

namespace Ubuntu {
namespace Internal {

QLatin1String UBUNTU_TARGET_ARCH_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.Arch");
QLatin1String UBUNTU_TARGET_FRAMEWORK_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.Framework");
QLatin1String UBUNTU_TARGET_SERIES_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.Series");
QLatin1String UBUNTU_TARGET_MINOR_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.MinorV");
QLatin1String UBUNTU_TARGET_MAJOR_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.MajorV");

static QMap <QString,ProjectExplorer::Abi::Architecture> init_architectures()
{
    QMap<QString,ProjectExplorer::Abi::Architecture> map;
    map.insert(QLatin1String("armhf")  ,ProjectExplorer::Abi::ArmArchitecture);
    map.insert(QLatin1String("i386") ,ProjectExplorer::Abi::X86Architecture);
    map.insert(QLatin1String("amd64"),ProjectExplorer::Abi::X86Architecture);
    return map;
}

QMap <QString,ProjectExplorer::Abi::Architecture> clickArchitectures = init_architectures();


QList<Utils::FileName> ClickToolChain::suggestedMkspecList() const
{
    return QList<Utils::FileName>()<< Utils::FileName::fromString(QLatin1String("linux-g++"));
}

Utils::FileName ClickToolChain::suggestedDebugger() const
{
    return Utils::FileName::fromString(QLatin1String("/usr/bin/gdb-multiarch"));
}

QString ClickToolChain::type() const
{
    return QString::fromLatin1(Constants::UBUNTU_CLICK_TOOLCHAIN_ID);
}

QString ClickToolChain::typeDisplayName() const
{
    return ClickToolChainFactory::tr("Ubuntu GCC");
}

bool ClickToolChain::isValid() const
{
    return GccToolChain::isValid() && targetAbi().isValid() && UbuntuClickTool::targetExists(m_clickTarget);
}

void ClickToolChain::addToEnvironment(Utils::Environment &env) const
{
    GccToolChain::addToEnvironment(env);
    env.set(QLatin1String("CLICK_SDK_ARCH")     , m_clickTarget.architecture);
    env.set(QLatin1String("CLICK_SDK_FRAMEWORK"), m_clickTarget.framework);
    env.set(QLatin1String("CLICK_SDK_SERIES")   , m_clickTarget.series);
}

QString ClickToolChain::makeCommand(const Utils::Environment &) const
{
    QString command = QString::fromLatin1(Constants::UBUNTU_CLICK_MAKE_WRAPPER).arg(Constants::UBUNTU_SCRIPTPATH);
    return command;
}

bool ClickToolChain::operator ==(const ProjectExplorer::ToolChain &tc) const
{
    if (!GccToolChain::operator ==(tc))
        return false;

    return (m_clickTarget.architecture == m_clickTarget.architecture
            && m_clickTarget.framework == m_clickTarget.framework
            && m_clickTarget.series == m_clickTarget.series);
}

ProjectExplorer::ToolChainConfigWidget *ClickToolChain::configurationWidget()
{
    return GccToolChain::configurationWidget();
}

const UbuntuClickTool::Target &ClickToolChain::clickTarget() const
{
    return m_clickTarget;
}

QVariantMap ClickToolChain::toMap() const
{
    QVariantMap map = GccToolChain::toMap();
    map.insert(UBUNTU_TARGET_ARCH_KEY,m_clickTarget.architecture);
    map.insert(UBUNTU_TARGET_FRAMEWORK_KEY,m_clickTarget.framework);
    map.insert(UBUNTU_TARGET_SERIES_KEY,m_clickTarget.series);
    map.insert(UBUNTU_TARGET_MAJOR_KEY,m_clickTarget.majorVersion);
    map.insert(UBUNTU_TARGET_MINOR_KEY,m_clickTarget.minorVersion);

    return map;
}

bool ClickToolChain::fromMap(const QVariantMap &data)
{
    if(!GccToolChain::fromMap(data))
        return false;

    if(!data.contains(UBUNTU_TARGET_ARCH_KEY)
            || !data.contains(UBUNTU_TARGET_FRAMEWORK_KEY)
            || !data.contains(UBUNTU_TARGET_SERIES_KEY)
            || !data.contains(UBUNTU_TARGET_MAJOR_KEY)
            || !data.contains(UBUNTU_TARGET_MINOR_KEY))
        return false;

    m_clickTarget.architecture = data[UBUNTU_TARGET_ARCH_KEY].toString();
    m_clickTarget.framework    = data[UBUNTU_TARGET_FRAMEWORK_KEY].toString();
    m_clickTarget.series       = data[UBUNTU_TARGET_SERIES_KEY].toString();
    m_clickTarget.majorVersion = data[UBUNTU_TARGET_MAJOR_KEY].toInt();
    m_clickTarget.minorVersion = data[UBUNTU_TARGET_MINOR_KEY].toInt();

    //only valid click targets are used for Kit creation
    m_clickTarget.maybeBroken  = false;

    fixAbi();

    return isValid();
}

Utils::FileName ClickToolChain::compilerCommand() const
{
    return GccToolChain::compilerCommand();
}

ClickToolChain::ClickToolChain(const UbuntuClickTool::Target &target, Detection d)
    : GccToolChain(QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID), d)
    , m_clickTarget(target)
{
    setCompilerCommand(Utils::FileName::fromString(
                           QString::fromLatin1(Constants::UBUNTU_CLICK_GCC_WRAPPER)
                           .arg(Constants::UBUNTU_SCRIPTPATH)));
    fixAbi();
    setDisplayName(QString::fromLatin1("Ubuntu GCC (%1-%2-%3)")
                   .arg(ProjectExplorer::Abi::toString(targetAbi().architecture()))
                   .arg(target.framework)
                   .arg(target.series));
}

ClickToolChain::ClickToolChain(const ClickToolChain &other)
    : GccToolChain(other)
    , m_clickTarget(other.m_clickTarget)
{
    fixAbi();
}

ClickToolChain::ClickToolChain()
    : GccToolChain(QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID),ManualDetection)
{
}

/*!
 * \brief ClickToolChain::fixAbi
 * fixes the internal abi, we just point to a script
 * executing gcc
 */
void ClickToolChain::fixAbi()
{
    ProjectExplorer::Abi abi = ProjectExplorer::Abi(clickArchitectures[m_clickTarget.architecture], ProjectExplorer::Abi::LinuxOS,
            ProjectExplorer::Abi::GenericLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
            32);

    setTargetAbi(abi);
}

ClickToolChainFactory::ClickToolChainFactory()
{
    setId(Constants::UBUNTU_CLICK_TOOLCHAIN_ID);
    setDisplayName(tr("Ubuntu GCC"));
}

QList<ProjectExplorer::ToolChain *> ClickToolChainFactory::autoDetect()
{
    return createToolChainsForClickTargets();
}

bool ClickToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ProjectExplorer::ToolChain *ClickToolChainFactory::restore(const QVariantMap &data)
{
    ClickToolChain *tc = new ClickToolChain();
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

QList<ProjectExplorer::ToolChain *> ClickToolChainFactory::createToolChainsForClickTargets()
{
    QList<ProjectExplorer::ToolChain*> toolChains;

    QList<UbuntuClickTool::Target> targets = UbuntuClickTool::listAvailableTargets();
    foreach(const UbuntuClickTool::Target &target, targets) {
        qDebug()<<"Found Target"<<target;
        if(!clickArchitectures.contains(target.architecture)
                || target.maybeBroken)
            continue;

        ClickToolChain* tc = new ClickToolChain(target, ProjectExplorer::ToolChain::AutoDetection);
        toolChains.append(tc);
    }

    return toolChains;
}

} // namespace Internal
} // namespace Ubuntu
