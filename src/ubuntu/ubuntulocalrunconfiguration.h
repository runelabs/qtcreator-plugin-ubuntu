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

#ifndef UBUNTURUNCONFIGURATION_H
#define UBUNTURUNCONFIGURATION_H

#include <QObject>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <utils/fileutils.h>


namespace ProjectExplorer {
class Target;
}


namespace Ubuntu {
namespace Internal {

class UbuntuClickManifest;

class UbuntuLocalRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
public:
    UbuntuLocalRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    UbuntuLocalRunConfiguration(ProjectExplorer::Target *parent, UbuntuLocalRunConfiguration* source);

    QWidget *createConfigurationWidget() override;
    bool isEnabled() const override;

    QString appId() const;

    // LocalApplicationRunConfiguration interface
    virtual QString executable() const override;
    virtual QString workingDirectory() const override;
    virtual QString commandLineArguments() const override;
    virtual ProjectExplorer::ApplicationLauncher::Mode runMode() const override;
    virtual void addToBaseEnvironment(Utils::Environment &env) const override;

    // RunConfiguration interface
    virtual bool isConfigured () const override;
    virtual ConfigurationState ensureConfigured(QString *errorMessage) override;

    //static helpers
    static QString getDesktopFile (RunConfiguration *config, QString appId, QString *errorMessage = 0);
    static bool readDesktopFile (const QString &desktopFile, QString *executable, QStringList *arguments, QString *errorMessage);

private:
    bool ensureClickAppConfigured (QString *errorMessage);
    bool ensureScopesAppConfigured (QString *errorMessage);
    bool ensureUbuntuProjectConfigured (QString *errorMessage);

private:
    QString m_executable;
    Utils::FileName m_workingDir;
    QStringList m_args;
};

}
}

#endif // UBUNTURUNCONFIGURATION_H
