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
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#include "ubuntuwelcomemode.h"
#include "ubuntuconstants.h"

#include <QQmlEngine>
#include <QQmlContext>

#include <utils/fileutils.h>
#include <utils/algorithm.h>
#include <coreplugin/iwizardfactory.h>
#include <projectexplorer/projectexplorer.h>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QDir>
#include <QDebug>
#include <QList>
#include <QCoreApplication>
#include <QProcess>

using namespace Ubuntu;
using namespace Ubuntu::Internal;

QUrl UbuntuWelcomePage::pageLocation() const
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    const QString resourcePath = Utils::FileUtils::normalizePathName(Constants::UBUNTU_WELCOMESCREEN_QML);
    return QUrl::fromLocalFile(resourcePath);
}

QString UbuntuWelcomePage::title() const
{
    return tr("Ubuntu-SDK");
}

int UbuntuWelcomePage::priority() const
{
    return 0;
}

void UbuntuWelcomePage::facilitateQml(QQmlEngine *engine)
{
    engine->setOutputWarningsToStandardError(true);
    QQmlContext *context = engine->rootContext();
    context->setContextProperty(QLatin1String("ubuntuWelcomeMode"), this);
}

Core::Id UbuntuWelcomePage::id() const
{
    return "UbuntuSdkPage";
}

void UbuntuWelcomePage::newProject()
{
    Core::ICore::showNewItemDialog(tr("New Project"), Utils::filtered(Core::IWizardFactory::allWizardFactories(),
                                                                      [](Core::IWizardFactory *f) {
                                                                          return f->kind() == Core::IWizardFactory::ProjectWizard;
                                                                      }));
}

void UbuntuWelcomePage::openProject()
{
    ProjectExplorer::ProjectExplorerPlugin::instance()->openOpenProjectDialog();
}

void UbuntuWelcomePage::openGallery()
{
    QProcess::startDetached(QString::fromLatin1("%1/qtc_launch_gallery").arg(Constants::UBUNTU_SCRIPTPATH));
}
