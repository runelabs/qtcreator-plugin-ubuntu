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

#ifndef UBUNTU_INTERNAL_UBUNTUDEPLOYCONFIGURATION_H
#define UBUNTU_INTERNAL_UBUNTUDEPLOYCONFIGURATION_H

#include <projectexplorer/deployconfiguration.h>
#include <remotelinux/abstractremotelinuxdeploystep.h>

namespace RemoteLinux {
class GenericDirectUploadService;
}

namespace Ubuntu {
namespace Internal {

class UbuntuDirectUploadStep : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_OBJECT

public:
    UbuntuDirectUploadStep(ProjectExplorer::BuildStepList *bsl);
    UbuntuDirectUploadStep(ProjectExplorer::BuildStepList *bsl, UbuntuDirectUploadStep *other);
    ~UbuntuDirectUploadStep();

    // BuildStep interface
    virtual void run(QFutureInterface<bool> &fi);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool initInternal(QString *error = 0);

    RemoteLinux::AbstractRemoteLinuxDeployService *deployService() const;
    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    static Core::Id stepId();
    static QString displayName();

private slots:
    void projectNameChanged();

private:
    RemoteLinux::GenericDirectUploadService *m_deployService;
};

class UbuntuDeployStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    // IBuildStepFactory interface
    virtual QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    virtual QString displayNameForId(const Core::Id id) const;
    virtual bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    virtual bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    virtual bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    virtual ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

class UbuntuRemoteDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit UbuntuRemoteDeployConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;
    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *product);
};
} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUDEPLOYCONFIGURATION_H
