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

#include "ubuntuwebmode.h"
#include "ubuntuconstants.h"

#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <utils/styledbar.h>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFile>
#include <QWebView>

using namespace Ubuntu::Internal;

UbuntuWebMode::UbuntuWebMode(QObject *parent) :
    Core::IMode(parent)
{
    setDisplayName(tr(Ubuntu::Constants::UBUNTU_MODE_WEB_DISPLAYNAME));
    setIcon(QIcon(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_WEB_ICON)));
    setPriority(Ubuntu::Constants::UBUNTU_MODE_WEB_PRIORITY);
    setId(Ubuntu::Constants::UBUNTU_MODE_WEB);
    setObjectName(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_WEB));


    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_modeWidget.setLayout(layout);

    connect(&m_webView,SIGNAL(urlChanged(QUrl)),SLOT(updateAddress(QUrl)));
    connect(&m_addressBar,SIGNAL(returnPressed()),SLOT(goToAddress()));

    //fit into look and feel
    Utils::StyledBar* styledBar = new Utils::StyledBar(&m_modeWidget);
    layout->addWidget(styledBar);

    QScrollArea *scrollArea = new QScrollArea(&m_modeWidget);
    scrollArea->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scrollArea);
    layout->addWidget(&m_addressBar);
    scrollArea->setWidget(&m_webView);
    scrollArea->setWidgetResizable(true);

    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)), SLOT(modeChanged(Core::IMode*)));

    m_webView.setFocus();
    setWidget(&m_modeWidget);
}

void UbuntuWebMode::updateAddress(QUrl url) {
    m_addressBar.setText(url.toString());
}

void UbuntuWebMode::goToAddress() {
    m_webView.setUrl(QUrl::fromUserInput(m_addressBar.text()));
}

void UbuntuWebMode::initialize() {

}

QUrl UbuntuWebMode::url() const
{
    return m_webView.url();
}

void UbuntuWebMode::modeChanged(Core::IMode*) {

}

