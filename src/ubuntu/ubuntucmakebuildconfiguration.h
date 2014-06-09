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
#ifndef UBUNTU_INTERNAL_UBUNTUCMAKEBUILDCONFIGURATION_H
#define UBUNTU_INTERNAL_UBUNTUCMAKEBUILDCONFIGURATION_H

#include <cmakeprojectmanager/cmakebuildconfiguration.h>
#include <cmakeprojectmanager/makestep.h>
#include <projectexplorer/namedwidget.h>

#include "ubuntucmakemakestep.h"


namespace Utils{
    class PathChooser;
}
namespace CMakeProjectManager{
    class ArgumentsLineEdit;
}

namespace Ubuntu {
namespace Internal {

class UbuntuCMakeBuildConfiguration : public CMakeProjectManager::CMakeBuildConfiguration
{
    Q_OBJECT
public:
    UbuntuCMakeBuildConfiguration(ProjectExplorer::Target *parent);
    virtual ProjectExplorer::NamedWidget *createConfigWidget() override;
protected:
    UbuntuCMakeBuildConfiguration(ProjectExplorer::Target *parent, UbuntuCMakeBuildConfiguration *source);
    virtual bool fromMap(const QVariantMap &map) override;

    friend class UbuntuCMakeBuildConfigurationFactory;
};

class UbuntuCMakeBuildConfigurationFactory : public CMakeProjectManager::CMakeBuildConfigurationFactory
{
    Q_OBJECT
public:
    UbuntuCMakeBuildConfigurationFactory(QObject *parent = 0);
    ~UbuntuCMakeBuildConfigurationFactory();

    // IBuildConfigurationFactory interface
    virtual int priority(const ProjectExplorer::Target *parent) const override;
    virtual QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    virtual int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    virtual QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    virtual UbuntuCMakeBuildConfiguration *create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const override;
    virtual bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    virtual UbuntuCMakeBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
    virtual bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const override;
    virtual UbuntuCMakeBuildConfiguration*clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) override;
private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

class UbuntuCMakeBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT
public:
    UbuntuCMakeBuildSettingsWidget(UbuntuCMakeBuildConfiguration *bc);

private slots:
    void onArgumentsChanged();
    void onBuilddirChanged();
private:
    Utils::PathChooser *m_pathChooser;
    CMakeProjectManager::ArgumentsLineEdit *m_userArguments;
    UbuntuCMakeBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCMAKEBUILDCONFIGURATION_H
