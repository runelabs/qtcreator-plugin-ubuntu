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


#ifndef UBUNTUSETTINGSCLICKWIDGET_H
#define UBUNTUSETTINGSCLICKWIDGET_H

#include <QWidget>
#include <QSignalMapper>
#include "ubuntuclicktool.h"

namespace Ui {
class UbuntuSettingsClickWidget;
}

namespace Ubuntu {
namespace Internal {

class UbuntuSettingsClickWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UbuntuSettingsClickWidget(QWidget *parent = 0);
    ~UbuntuSettingsClickWidget();
    void apply();

protected slots:
    void on_pushButtonCreateClickTarget_clicked();
    void on_deleteClickChroot (const int index);
    void on_maintainClickChroot (const int index);
    void on_upgradeClickChroot (const int index);
    void on_toggleTargetUpgradeEnabled (const int index);

private:
    void listExistingClickTargets ();

private:
    Ui::UbuntuSettingsClickWidget *ui;
    QSignalMapper* m_deleteMapper;
    QSignalMapper* m_updateMapper;
    QSignalMapper* m_maintainMapper;
    QSignalMapper* m_toggleUpgradeMapper;
    QList<Ubuntu::UbuntuClickTool::Target> m_availableTargets;
};

}}

#endif // UBUNTUSETTINGSCLICKWIDGET_H
