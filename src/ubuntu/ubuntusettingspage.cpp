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

#include "ubuntusettingspage.h"

using namespace Ubuntu::Internal;
using namespace Ubuntu;

UbuntuSettingsPage::UbuntuSettingsPage() :
    m_widget(0)
{
    setId("A.General");
    setDisplayName(tr("General"));
    setCategory("Ubuntu");
    setDisplayCategory(QLatin1String("Ubuntu"));
    setCategoryIcon(QLatin1String(Constants::UBUNTU_SETTINGS_ICON));
}

UbuntuSettingsPage::~UbuntuSettingsPage()
{
}

QWidget *UbuntuSettingsPage::createPage(QWidget *parent)
{
    m_widget = new UbuntuSettingsWidget(parent);
    return m_widget;
}

void UbuntuSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;

    m_widget->apply();
}
