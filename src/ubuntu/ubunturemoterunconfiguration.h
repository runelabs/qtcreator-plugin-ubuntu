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
    virtual QString localExecutableFilePath() const;
    virtual QString remoteExecutableFilePath() const;
    virtual QStringList arguments() const;
    virtual QString workingDirectory() const;
    virtual QString alternateRemoteExecutable() const;
    virtual bool useAlternateExecutable() const;
    virtual Utils::Environment environment() const;
    virtual QStringList soLibSearchPaths () const;

    // RunConfiguration interface
    virtual QWidget *createConfigurationWidget();
    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    virtual bool isConfigured() const;
    virtual bool ensureConfigured(QString *errorMessage);

    // ProjectConfiguration interface
    virtual bool fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

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
