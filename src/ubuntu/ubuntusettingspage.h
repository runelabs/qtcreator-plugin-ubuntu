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

#ifndef UBUNTUSETTINGSPAGE_H
#define UBUNTUSETTINGSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>
#include "ubuntuconstants.h"
#include "ubuntusettingswidget.h"
#include <QPointer>

namespace Ubuntu {
    namespace Internal {
        class UbuntuSettingsPage : public Core::IOptionsPage
        {
            Q_OBJECT

        public:
            explicit UbuntuSettingsPage();
            ~UbuntuSettingsPage();

            QWidget *createPage(QWidget *parent);
            void apply();
            void finish() { }

        protected:
            QPointer<UbuntuSettingsWidget> m_widget;
        };
    }
}

#endif // UBUNTUSETTINGSPAGE_H
