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

#ifndef UBUNTUWELCOMEMODE_H
#define UBUNTUWELCOMEMODE_H

#include <coreplugin/imode.h>
#include <QDeclarativeView>
#include <QObject>

namespace Ubuntu {
namespace Internal {

class UbuntuWelcomeMode : public Core::IMode
{
    Q_OBJECT

public:
    UbuntuWelcomeMode(QObject *parent = 0);
    void initialize();

public slots:
    void newProject();
    void openProject();

protected slots:
    void modeChanged(Core::IMode*);
    void objectAdded(QObject* obj);

protected:
    QDeclarativeView* m_declarativeView;
    QWidget* m_modeWidget;
    QList<QObject*> m_welcomeTabPluginList;
};


} // Internal
} // Ubuntu

#endif // UBUNTUWELCOMEMODE_H
