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

#include "ubuntusettingsclickpage.h"
#include "ubuntuconstants.h"

using namespace Ubuntu::Internal;

UbuntuSettingsClickPage::UbuntuSettingsClickPage() :
    m_widget(0)
{
    setId("A.Click");
    setDisplayName(tr("Click"));
    setCategory("Ubuntu");
    setDisplayCategory(QLatin1String("Ubuntu"));
    setCategoryIcon(QLatin1String(Ubuntu::Constants::UBUNTU_SETTINGS_ICON));
}

UbuntuSettingsClickPage::~UbuntuSettingsClickPage()
{
}

QWidget *UbuntuSettingsClickPage::widget( )
{
    if(!m_widget)
        m_widget = new UbuntuSettingsClickWidget();
    return m_widget;
}

void UbuntuSettingsClickPage::apply()
{
    if (!m_widget) // page was never shown
        return;

    m_widget->apply();
}

void UbuntuSettingsClickPage::finish()
{
    if (m_widget)
        delete m_widget;
}
