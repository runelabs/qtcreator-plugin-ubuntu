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

#include "ubunturunconfiguration.h"
#include "ubuntuproject.h"
#include "ubuntuprojectguesser.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/environment.h>

using namespace Ubuntu::Internal;

UbuntuRunConfiguration::UbuntuRunConfiguration(ProjectExplorer::Target *parent, Core::Id id)
    : ProjectExplorer::LocalApplicationRunConfiguration(parent, id)
{
    setDisplayName(parent->project()->displayName());
    addExtraAspect(new ProjectExplorer::LocalEnvironmentAspect(this));
}

UbuntuRunConfiguration::UbuntuRunConfiguration(ProjectExplorer::Target *parent, UbuntuRunConfiguration *source)
    : ProjectExplorer::LocalApplicationRunConfiguration(parent,source)
{
}

QWidget *UbuntuRunConfiguration::createConfigurationWidget()
{
    return NULL;
}

bool UbuntuRunConfiguration::isEnabled() const
{
    return true;
}

QString UbuntuRunConfiguration::executable() const
{
    UbuntuProject* ubuntuProject = qobject_cast<UbuntuProject*>(target()->project());
    if(ubuntuProject) {
        if (ubuntuProject->mainFile().compare(QString::fromLatin1("www/index.html"), Qt::CaseInsensitive) == 0) {
            Utils::Environment env = Utils::Environment::systemEnvironment();
            return env.searchInPath(QString::fromLatin1(Ubuntu::Constants::UBUNTUHTML_PROJECT_LAUNCHER_EXE));
        } else {
            return QtSupport::QtKitInformation::qtVersion(target()->kit())->qmlsceneCommand();
        }
    }

    if(UbuntuProjectGuesser::isScopesProject(target()->project())) {
        Utils::Environment env = Utils::Environment::systemEnvironment();
        return env.searchInPath(QString::fromLatin1(Ubuntu::Constants::UBUNTUSCOPES_PROJECT_LAUNCHER_EXE));
    }

    return QString();
}

QString UbuntuRunConfiguration::workingDirectory() const
{
    UbuntuProject* ubuntuProject = qobject_cast<UbuntuProject*>(target()->project());
    if(ubuntuProject)
        return ubuntuProject->projectDirectory();

    if(UbuntuProjectGuesser::isScopesProject(target()->project()))
        return target()->activeBuildConfiguration()->buildDirectory().toString();

    return QString();
}

QString UbuntuRunConfiguration::commandLineArguments() const
{
    UbuntuProject* ubuntuProject = qobject_cast<UbuntuProject*>(target()->project());
    if (ubuntuProject) {
        if (ubuntuProject->mainFile().compare(QString::fromLatin1("www/index.html"), Qt::CaseInsensitive) == 0) {
            return QString(QLatin1String("--www=%0/www --inspector")).arg(ubuntuProject->projectDirectory());
        } else {
            return QString(QLatin1String("%0.qml")).arg(ubuntuProject->displayName());
        }
    }

    if(UbuntuProjectGuesser::isScopesProject(target()->project())) {
        Utils::FileName iniFile = UbuntuProjectGuesser::findScopesIniRecursive(target()->activeBuildConfiguration()->buildDirectory());

        if(iniFile.toFileInfo().exists())
            return QString(iniFile.toFileInfo().absoluteFilePath());
    }
    return QString();
}

QString UbuntuRunConfiguration::dumperLibrary() const
{
    return QtSupport::QtKitInformation::dumperLibrary(target()->kit());
}

QStringList UbuntuRunConfiguration::dumperLibraryLocations() const
{
    return QtSupport::QtKitInformation::dumperLibraryLocations(target()->kit());
}

ProjectExplorer::LocalApplicationRunConfiguration::RunMode UbuntuRunConfiguration::runMode() const
{
    return Gui;
}
