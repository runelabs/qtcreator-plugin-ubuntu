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

#include "ubuntupolicygroupinfo.h"

using namespace Ubuntu::Internal;

UbuntuPolicyGroupInfo::UbuntuPolicyGroupInfo(QObject *parent) :
    QObject(parent)
{
    connect(&m_process,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_process,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString,int)));
    connect(&m_process,SIGNAL(error(QString)),this,SLOT(onError(QString)));
}

void UbuntuPolicyGroupInfo::getInfo(QString policyGroup) {
    QStringList cmd;
    cmd << QString(QLatin1String("adb shell aa-easyprof --show-policy-group -p %0 --policy-vendor=ubuntu --policy-version=1.0")).arg(policyGroup);
    m_process.append(cmd);
    m_replies.clear();
    m_process.start(QLatin1String("Reading policy group"));
}

void UbuntuPolicyGroupInfo::onMessage(QString line) {
    m_replies.append(line.trimmed().replace(QLatin1String("\r"),QLatin1String("")).split(QLatin1String("\n")));
}

void UbuntuPolicyGroupInfo::onFinished(QString, int result) {
    emit infoReady(true);
}


void UbuntuPolicyGroupInfo::onError(QString) {
    m_replies.clear();
    emit infoReady(false);
}
