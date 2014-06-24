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

#include "ubuntuwelcomemode.h"
#include "ubuntuconstants.h"

#include <coreplugin/modemanager.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <utils/styledbar.h>
#include <utils/iwelcomepage.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/dialogs/iwizard.h>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QDir>
#include <QDebug>
#include <QList>
#include <QCoreApplication>

using namespace Ubuntu;
using namespace Ubuntu::Internal;


UbuntuWelcomeMode::UbuntuWelcomeMode(QObject *parent) : Core::IMode(parent),
                                                        m_quickView(new QQuickView) {
    setDisplayName(tr(Ubuntu::Constants::UBUNTU_MODE_WELCOME_DISPLAYNAME));
    setIcon(QIcon(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_WELCOME_ICON)));
    setPriority(Core::Constants::P_MODE_WELCOME);
    setId(Ubuntu::Constants::UBUNTU_MODE_WELCOME);
    setObjectName(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_WELCOME));

    QQmlContext *context = m_quickView->rootContext();
    context->setContextProperty(QLatin1String("welcomeMode"), this);

    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_quickView->setMinimumWidth(860);
    m_quickView->setMinimumHeight(548);

    m_modeWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_modeWidget->setLayout(layout);

    Utils::StyledBar* styledBar = new Utils::StyledBar(m_modeWidget);
    layout->addWidget(styledBar);
    QScrollArea *scrollArea = new QScrollArea(m_modeWidget);
    scrollArea->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scrollArea);

    QWidget* container = QWidget::createWindowContainer(m_quickView);

    scrollArea->setWidget(container);
    scrollArea->setWidgetResizable(true);
    container->setMinimumWidth(860);
    container->setMinimumHeight(548);
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)), SLOT(modeChanged(Core::IMode*)));
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    connect(pluginManager, SIGNAL(objectAdded(QObject*)), SLOT(objectAdded(QObject*)));

    setWidget(m_modeWidget);
    //qDebug() << __PRETTY_FUNCTION__;
}

void UbuntuWelcomeMode::modeChanged(Core::IMode *mode) {
    Q_UNUSED(mode);
}

void UbuntuWelcomeMode::initialize() {

    //qDebug() << __PRETTY_FUNCTION__;
    QQmlContext *context = m_quickView->rootContext();
    context->setContextProperty(QLatin1String("pagesModel"), QVariant::fromValue(m_welcomeTabPluginList));

    m_quickView->setSource(QUrl::fromLocalFile(Constants::UBUNTU_WELCOMESCREEN_QML));
    // Load existing welcome screen plugins - start
 /*   QList<Utils::IWelcomePage*> loadedWelcomeScreenPlugins = ExtensionSystem::PluginManager::getObjects<Utils::IWelcomePage>();

    QDeclarativeEngine *engine = m_declarativeView->engine();
    

    QStringList importPathList = engine->importPathList();
    importPathList << Core::ICore::resourcePath() + QLatin1String("/welcomescreen")
                   << QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String("/../" IDE_LIBRARY_BASENAME "/qtcreator"));                   
    engine->setImportPathList(importPathList);
    qDebug() << engine->importPathList();
    qDebug() << "Checking for loaded plugins";
    foreach (Utils::IWelcomePage *plugin, loadedWelcomeScreenPlugins) {
        qDebug() << "Load welcomepage plugin: " << plugin->title();
        plugin->facilitateQml(engine);
        m_welcomeTabPluginList.append(plugin);
    }
    context->setContextProperty(QLatin1String("pagesModel"), QVariant::fromValue(m_welcomeTabPluginList));
    qDebug() << "check complete";*/
    // end

}

void UbuntuWelcomeMode::objectAdded(QObject* obj) {
    Q_UNUSED(obj);
    //qDebug() << __PRETTY_FUNCTION__;
}


void UbuntuWelcomeMode::newProject() {
    Core::ICore::showNewItemDialog(tr("New Project"), Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
}

void UbuntuWelcomeMode::openProject() {
    ProjectExplorer::ProjectExplorerPlugin::instance()->openOpenProjectDialog();
}
