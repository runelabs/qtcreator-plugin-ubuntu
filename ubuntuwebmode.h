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
#ifndef UBUNTUWEBMODE_H
#define UBUNTUWEBMODE_H

#include <coreplugin/imode.h>
#include <QObject>
#include <QWebView>
#include <QLineEdit>

namespace Ubuntu {
namespace Internal {

class UbuntuWebMode : public Core::IMode
{
    Q_OBJECT

public:
    explicit UbuntuWebMode(QObject *parent = 0);
    void initialize();

public slots:
    void setUrl(QUrl url) { m_webView.setUrl(url); }

protected slots:
    void modeChanged(Core::IMode*);
    void updateAddress(QUrl url);
    void goToAddress();

protected:
    QWidget m_modeWidget;
    QWebView m_webView;
    QLineEdit m_addressBar;
};

}
}

#endif // UBUNTUWEBMODE_H
