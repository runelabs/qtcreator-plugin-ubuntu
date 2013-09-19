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

#include "ubuntupolicygroupmodel.h"
#include <QProcess>

using namespace Ubuntu::Internal;

UbuntuPolicyGroupModel::UbuntuPolicyGroupModel(QObject *parent) :
    QStringListModel(parent), m_bLocal(false)
{
    connect(&m_process,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_process,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString,int)));
    connect(&m_process,SIGNAL(error(QString)),this,SLOT(onError(QString)));
}

void UbuntuPolicyGroupModel::scanPolicyGroups() {
    QStringList cmd;
    if (!isLocal()) {
        cmd << QLatin1String("adb shell aa-easyprof --list-policy-groups --policy-vendor=ubuntu --policy-version=1.0");
    } else {
        cmd << QLatin1String("aa-easyprof --list-policy-groups --policy-vendor=ubuntu --policy-version=1.0");
    }
    m_process.append(cmd);
    m_process.start(QLatin1String("Scanning policy groups"));
}

void UbuntuPolicyGroupModel::onMessage(QString line) {
    m_replies.append(line.trimmed().replace(QLatin1String("\r"),QLatin1String("")).split(QLatin1String("\n")));
}

void UbuntuPolicyGroupModel::onFinished(QString, int result) {
    setStringList(m_replies);
    m_replies.clear();
    emit scanComplete(true);
}


void UbuntuPolicyGroupModel::onError(QString) {
    setStringList(m_replies);
    m_replies.clear();
    if (!isLocal()) {
        m_bLocal = true;
        scanPolicyGroups();
    } else {
        emit scanComplete(false);
    }
}
