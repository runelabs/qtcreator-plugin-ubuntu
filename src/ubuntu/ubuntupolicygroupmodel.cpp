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
    QStringListModel(parent),
    m_policyVersion(QStringLiteral("1.1")),
    m_bLocal(false)
{
    connect(&m_process,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(&m_process,SIGNAL(finished(QString,int)),this,SLOT(onFinished(QString,int)));
}

void UbuntuPolicyGroupModel::setPolicyVersion(const QString &policyVersion)
{
    if(!policyVersion.isEmpty())
        m_policyVersion = policyVersion;
}

QString UbuntuPolicyGroupModel::policyVersion() const
{
    return m_policyVersion;
}

void UbuntuPolicyGroupModel::scanPolicyGroups() {
    QStringList cmd;
    if (!isLocal()) {
        cmd << QString::fromLatin1("adb shell aa-easyprof --list-policy-groups --policy-vendor=ubuntu --policy-version=%1")
               .arg(m_policyVersion);
    } else {
        cmd << QString::fromLatin1("aa-easyprof --list-policy-groups --policy-vendor=ubuntu --policy-version=%1")
               .arg(m_policyVersion);
    }
    m_process.append(cmd);
    m_process.start(QLatin1String("Scanning policy groups"));
}

void UbuntuPolicyGroupModel::onMessage(QString line) {
    m_replies.append(line.trimmed().replace(QLatin1String("\r"),QLatin1String("")).split(QLatin1String("\n")));
}

void UbuntuPolicyGroupModel::onFinished(QString, int result) {
    if (result != 0 && !isLocal()) {
        //first try on the device failed, fall back to local
        m_bLocal = true;
        m_replies.clear();
        scanPolicyGroups();
        return;
    } else if ( result == 0) {
        //we got a result lets show it
        setStringList(m_replies);
        m_replies.clear();
        emit scanComplete(true);
        return;
    }

    //local and device failed, no result :/
    setStringList(m_replies);
    m_replies.clear();
    emit scanComplete(false);
}
