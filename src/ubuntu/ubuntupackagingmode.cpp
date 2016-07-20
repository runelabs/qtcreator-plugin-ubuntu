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

#include "ubuntupackagingmode.h"
#include "ubuntupackagingmodel.h"
#include "ubuntuconstants.h"

#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <utils/styledbar.h>
#include <QVBoxLayout>
#include <QScrollArea>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <QScrollArea>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>

using namespace Ubuntu::Internal;

UbuntuPackagingMode *UbuntuPackagingMode::m_instance = 0;

UbuntuPackagingMode::UbuntuPackagingMode(QObject *parent) :
    Core::IMode(parent)
{
    Q_ASSERT(m_instance == 0);
    m_instance = this;

    setDisplayName(tr(Ubuntu::Constants::UBUNTU_MODE_PACKAGING_DISPLAYNAME));
    setIcon(QIcon(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_PACKAGING_ICON)));
    setPriority(Ubuntu::Constants::UBUNTU_MODE_PACKAGING_PRIORITY);
    setId(Ubuntu::Constants::UBUNTU_MODE_PACKAGING);
    setObjectName(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_PACKAGING));

    m_modeWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_modeWidget->setLayout(layout);

    Utils::StyledBar* styledBar = new Utils::StyledBar(m_modeWidget);
    layout->addWidget(styledBar);

    m_modeView = new QQuickView;
    m_modeView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_viewModel = new UbuntuPackagingModel(m_modeView);

    QWidget* container = QWidget::createWindowContainer(m_modeView);
    container->setMinimumWidth(860);
    container->setMinimumHeight(548);
    container->setFocusPolicy(Qt::TabFocus);
    layout->addWidget(container);

    m_modeView->rootContext()->setContextProperty(QLatin1String("publishModel") ,m_viewModel);
    m_modeView->rootContext()->setContextProperty(QLatin1String("resourceRoot") ,Constants::UBUNTU_DEVICESCREEN_ROOT);
    m_modeView->setSource(QUrl::fromLocalFile(Constants::UBUNTU_PUBLISHSCREEN_QML));

    QObject* sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager,SIGNAL(projectAdded(ProjectExplorer::Project*)),SLOT(on_projectAdded(ProjectExplorer::Project*)));
    connect(sessionManager,SIGNAL(projectRemoved(ProjectExplorer::Project*)),SLOT(on_projectRemoved(ProjectExplorer::Project*)));
    connect(sessionManager,SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),SLOT(on_projectAdded(ProjectExplorer::Project*)));
    connect(m_viewModel,SIGNAL(reviewToolsInstalledChanged(bool)),this,SLOT(updateModeState()));
    setWidget(m_modeWidget);
    setEnabled(false);
}

void UbuntuPackagingMode::initialize() {

}

void UbuntuPackagingMode::updateModeState() {
    ProjectExplorer::Project* startupProject = ProjectExplorer::SessionManager::startupProject();

    bool isQmlProject = false;
    //bool isQmakeProject = false;
    bool isUbuntuProject = false;
    bool isCMakeProject = false;
    bool isGoProject = false;
    bool reviewToolsInstalled = m_viewModel->reviewToolsInstalled();

    if (startupProject) {
        isQmlProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::QMLPROJECT_MIMETYPE));
        //isQmakeProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::QMAKE_MIMETYPE));
        isUbuntuProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::UBUNTUPROJECT_MIMETYPE));
        isCMakeProject  = (startupProject->projectManager()->mimeType() == QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE));
        isGoProject = (startupProject->projectManager()->mimeType() == QLatin1String(Constants::GO_PROJECT_MIMETYPE));
    }

    this->setEnabled((isQmlProject || isUbuntuProject || isCMakeProject || isGoProject || reviewToolsInstalled));
}

void UbuntuPackagingMode::on_projectAdded(ProjectExplorer::Project *project) {
    Q_UNUSED(project);
    updateModeState();
}

void UbuntuPackagingMode::on_projectRemoved(ProjectExplorer::Project *project) {
    Q_UNUSED(project);
    updateModeState();
}
