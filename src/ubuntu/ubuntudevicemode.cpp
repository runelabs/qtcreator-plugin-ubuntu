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

#include "ubuntudevicemode.h"
#include "ubuntuconstants.h"
#include "ubuntudevicesmodel.h"
#include "ubuntuemulatormodel.h"

#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <utils/styledbar.h>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>

using namespace Ubuntu::Internal;

UbuntuDeviceMode *UbuntuDeviceMode::m_instance = 0;

UbuntuDeviceMode::UbuntuDeviceMode(QObject *parent) :
    Core::IMode(parent)
{
    Q_ASSERT_X(m_instance == 0, Q_FUNC_INFO,"There can be only one instance of UbuntuDeviceMode");
    m_instance = this;

    setDisplayName(tr(Ubuntu::Constants::UBUNTU_MODE_DEVICES_DISPLAYNAME));
    setIcon(QIcon(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_DEVICES_ICON)));
    setPriority(Ubuntu::Constants::UBUNTU_MODE_DEVICES_PRIORITY);
    setId(Ubuntu::Constants::UBUNTU_MODE_DEVICES);
    setObjectName(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_DEVICES));


    m_modeWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_modeWidget->setLayout(layout);

    Utils::StyledBar* styledBar = new Utils::StyledBar(m_modeWidget);
    layout->addWidget(styledBar);

    m_modeView = new QQuickView;
    m_modeView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_devicesModel = new UbuntuDevicesModel(m_modeView);
    m_emulatorModel = new UbuntuEmulatorModel(m_modeView);

    QWidget* container = QWidget::createWindowContainer(m_modeView);
    container->setMinimumWidth(860);
    container->setMinimumHeight(548);
    container->setFocusPolicy(Qt::TabFocus);
    layout->addWidget(container);

    m_modeView->rootContext()->setContextProperty(QLatin1String("devicesModel"),m_devicesModel);
    m_modeView->rootContext()->setContextProperty(QLatin1String("emulatorModel"),m_emulatorModel);
    m_modeView->rootContext()->setContextProperty(QLatin1String("deviceMode")  ,this);
    m_modeView->rootContext()->setContextProperty(QLatin1String("resourceRoot")  ,Constants::UBUNTU_DEVICESCREEN_ROOT);
    m_modeView->setSource(QUrl::fromLocalFile(Constants::UBUNTU_DEVICESCREEN_QML));

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)), SLOT(modeChanged(Core::IMode*)));
    setWidget(m_modeWidget);
}

UbuntuDevice::ConstPtr UbuntuDeviceMode::device()
{
    if(m_devicesModel->rowCount() <= 0)
        return UbuntuDevice::ConstPtr();

    if(!m_deviceIndex.isValid()) {
        m_deviceIndex = 0; //device 0 is always the first selected
    }
    return m_devicesModel->device(m_deviceIndex.toInt());
}

void UbuntuDeviceMode::deviceSelected(const QVariant index)
{
    m_deviceIndex = index;
    emit updateDeviceActions ();
}

void UbuntuDeviceMode::modeChanged(Core::IMode *mode)
{
    Q_UNUSED(mode);
}

void UbuntuDeviceMode::initialize() {

}

UbuntuDeviceMode *UbuntuDeviceMode::instance()
{
    return m_instance;
}
