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

#ifndef SECURITYPOLICYPICKERDIALOG_H
#define SECURITYPOLICYPICKERDIALOG_H

#include <QDialog>
#include "ubuntupolicygroupmodel.h"
#include "ubuntupolicygroupinfo.h"

using namespace Ubuntu::Internal;

namespace Ui {
class UbuntuSecurityPolicyPickerDialog;
}

class UbuntuSecurityPolicyPickerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UbuntuSecurityPolicyPickerDialog(QWidget *parent = 0);
    ~UbuntuSecurityPolicyPickerDialog();

    QStringList selectedPolicyGroups();

protected slots:
    void on_pushButtonCancel_clicked();
    void on_pushButtonAdd_clicked();
    void onScanComplete(bool ok);
    void onInfoChanged(bool ok);
    void onPolicyClicked(QModelIndex);

private:
    Ui::UbuntuSecurityPolicyPickerDialog *ui;
    UbuntuPolicyGroupModel m_model;
    UbuntuPolicyGroupInfo m_info;
};


#endif // SECURITYPOLICYPICKERDIALOG_H
