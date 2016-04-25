/*
 * Copyright 2014 Canonical Ltd.
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

#ifndef UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H
#define UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H

#include <QDialog>
#include <QPair>
#include <QProcess>
#include "ubuntuclicktool.h"

namespace Ubuntu {
namespace Internal {

namespace Ui {
    class UbuntuCreateNewChrootDialog;
}

class UbuntuCreateNewChrootDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UbuntuCreateNewChrootDialog(const QString &arch = QString(), const QString &framework = QString(), QWidget *parent = 0);
    ~UbuntuCreateNewChrootDialog();

    static bool getNewChrootTarget(UbuntuClickTool::Target *target, const QString &arch, const QString &framework, QWidget *parent = 0);

protected:
    void load ();
    void loaderErrorOccurred(QProcess::ProcessError error);

protected slots:
    void loaderFinished();

private:
    QString m_filter;
    QProcess *m_loader;
    Ui::UbuntuCreateNewChrootDialog *ui;
};


} // namespace Internal
} // namespace Ubuntu
#endif // UBUNTU_INTERNAL_UBUNTUCREATENEWCHROOTDIALOG_H
