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
#ifndef UBUNTUSETTINGSDEVICECONNECTIVITYPAGE_H
#define UBUNTUSETTINGSDEVICECONNECTIVITYPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ubuntusettingsdeviceconnectivitywidget.h"
#include <QPointer>

namespace Ubuntu {
    namespace Internal {
        class UbuntuSettingsDeviceConnectivityPage : public Core::IOptionsPage
        {
            Q_OBJECT

        public:
            explicit UbuntuSettingsDeviceConnectivityPage();
            ~UbuntuSettingsDeviceConnectivityPage();

            QWidget *widget( ) override;
            void apply() override;
            void finish() override;

        protected:
            QPointer<UbuntuSettingsDeviceConnectivityWidget> m_widget;
        };
    }
}


#endif // UBUNTUSETTINGSDEVICECONNECTIVITYPAGE_H
