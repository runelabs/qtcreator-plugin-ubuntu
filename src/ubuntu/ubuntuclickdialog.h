/*
 * Copyright 2016 Canonical Ltd.
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
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */


#include <ubuntu/processoutputdialog.h>

namespace Ubuntu {
namespace Internal {

class UbuntuClickDialog : public ProcessOutputDialog
{

public:
    static bool createClickChrootModal (bool redetectKits = true , const QString &arch = QString(), const QString &framework = QString(), QWidget *parent = 0);
    static int maintainClickModal (const UbuntuClickTool::Target &target, const UbuntuClickTool::MaintainMode &mode);
    static int maintainClickModal (const QList<UbuntuClickTool::Target> &targetList, const UbuntuClickTool::MaintainMode &mode);
};

}}
