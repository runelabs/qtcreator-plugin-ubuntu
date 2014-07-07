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
#ifndef UBUNTU_INTERNAL_UBUNTUREMOTERUNCONFIGURATION_H
#define UBUNTU_INTERNAL_UBUNTUREMOTERUNCONFIGURATION_H

#include <remotelinux/abstractremotelinuxrunconfiguration.h>
#include <coreplugin/id.h>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteRunConfiguration : public RemoteLinux::AbstractRemoteLinuxRunConfiguration
{
    Q_OBJECT

public:
    UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent, UbuntuRemoteRunConfiguration *source);

    // AbstractRemoteLinuxRunConfiguration interface
public:
    virtual QString localExecutableFilePath() const override;
    virtual QString remoteExecutableFilePath() const override;
    virtual QStringList arguments() const override;
    virtual QString workingDirectory() const override;
    virtual Utils::Environment environment() const override;

    virtual QStringList soLibSearchPaths () const;

    // RunConfiguration interface
    virtual QWidget *createConfigurationWidget() override;
    virtual bool isEnabled() const override;
    virtual QString disabledReason() const override;
    virtual bool isConfigured() const override;
    virtual bool ensureConfigured(QString *errorMessage) override;

    // ProjectConfiguration interface
    virtual bool fromMap(const QVariantMap &map) override;
    virtual QVariantMap toMap() const override;

    QString appId () const;
    QString clickPackage () const;

    static Core::Id typeId ();
    void setArguments (const QStringList &args);

private:
    QString m_clickPackage;
    QString m_appId;
    QString m_desktopFile;
    QString m_localExecutable;
    QString m_remoteExecutable;
    QStringList m_arguments;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUREMOTERUNCONFIGURATION_H
