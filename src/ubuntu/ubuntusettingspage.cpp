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
#include "ubuntuconstants.h"

using namespace Ubuntu::Internal;
using namespace Ubuntu;

UbuntuSettingsPage::UbuntuSettingsPage() :
    m_widget(0)
{
    setId("A.Tabs");
    setDisplayName(tr("Tabs"));
    setCategory("Ubuntu");
    setDisplayCategory(QLatin1String("Ubuntu"));
    setCategoryIcon(QLatin1String(Ubuntu::Constants::UBUNTU_SETTINGS_ICON));
}

UbuntuSettingsPage::~UbuntuSettingsPage()
{
}

QWidget *UbuntuSettingsPage::widget()
{
    if(!m_widget)
        m_widget = new UbuntuSettingsWidget();

    return m_widget;
}

void UbuntuSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;

    m_widget->apply();
}

void UbuntuSettingsPage::finish()
{
    delete m_widget;
}
