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

#include "ubuntuapimode.h"
#include "ubuntuconstants.h"
#include "clicktoolchain.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QUrl>
#include <QUrlQuery>

using namespace Ubuntu::Internal;

const QString DEFAULT_PREFIX = QDir::separator()+QStringLiteral("usr");

UbuntuAPIMode::UbuntuAPIMode(QObject *parent) :
    UbuntuWebMode(parent)
{
    setDisplayName(tr(Ubuntu::Constants::UBUNTU_MODE_API_DISPLAYNAME));
    setId(Ubuntu::Constants::UBUNTU_MODE_API);
    setObjectName(QLatin1String(Ubuntu::Constants::UBUNTU_MODE_API));

    setUrl(defaultUrl());

    connect(ProjectExplorer::SessionManager::instance(),
            SIGNAL(startupProjectChanged(ProjectExplorer::Project *)),
            this,
            SLOT(startupProjectChanged(ProjectExplorer::Project*)));
}

void UbuntuAPIMode::modeChanged(Core::IMode *mode)
{
    if (mode == this) {
        activeTargetChanged();
    }
}

void UbuntuAPIMode::startupProjectChanged(ProjectExplorer::Project *startup)
{
    if(m_currProjectConnection)
        QObject::disconnect(m_currProjectConnection);

    m_currProjectConnection = connect(startup,
                                      SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                                      this,
                                      SLOT(activeTargetChanged()));
    activeTargetChanged();
}

void UbuntuAPIMode::activeTargetChanged()
{
    ProjectExplorer::Project *p = ProjectExplorer::SessionManager::startupProject();
    if(!p)
        return;

    ProjectExplorer::Target *t = p->activeTarget();
    if(!t)
        return;

    ProjectExplorer::Kit *k = t->kit();
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);

    QString prefix = DEFAULT_PREFIX;

    if(tc && tc->type() == QLatin1String(Constants::UBUNTU_CLICK_TOOLCHAIN_ID)){
        Utils::FileName sysrootPrefix = ProjectExplorer::SysRootKitInformation::sysRoot(k);
        if (!sysrootPrefix.isNull()) {
            prefix = sysrootPrefix.toString()+DEFAULT_PREFIX;
        }
    }

    QUrl current = url();
    QString loc;

    if(!current.isLocalFile())
        loc = QString::fromLatin1(Ubuntu::Constants::UBUNTU_API_OFFLINE).arg(prefix);
    else {
        loc = current.toLocalFile();

        //already good
        if(loc.startsWith(prefix))
            return;

        QRegularExpressionMatch match = QRegularExpression(QStringLiteral("^(.*?\\/usr)")).match(loc);
        if (match.hasMatch() ) {

            QString original = loc;

            //try to point to the current document in documentation
            loc = loc.replace(match.capturedStart(1),match.capturedLength(1),prefix);

            if(!QFile::exists(loc)) {
                //fall back to the index
                loc = QString::fromLatin1(Ubuntu::Constants::UBUNTU_API_OFFLINE).arg(prefix);
            }

            if(!QFile::exists(loc)){
                //try to point to the current document in host docs
                loc = original.replace(match.capturedStart(1),match.capturedLength(1),DEFAULT_PREFIX);
            }
        }
        else
            loc = QString();
    }

    if(loc.isEmpty() || !QFile::exists(loc))
        setUrl(defaultUrl());
    else {
        QUrl newUrl = QUrl::fromUserInput(loc);
        if(current.hasQuery()) {
            QUrlQuery query(current);
            QUrlQuery newQuery;
            foreach(auto item, query.queryItems())
                newQuery.addQueryItem(item.first,item.second);

            newUrl.setQuery(newQuery.toString());
        }

        if(current.hasFragment())
            newUrl.setFragment(current.fragment());

        setUrl(newUrl);
    }
}

QUrl UbuntuAPIMode::defaultUrl() const
{
    QString localDocs = QString::fromLatin1(Ubuntu::Constants::UBUNTU_API_OFFLINE).arg(DEFAULT_PREFIX);
    QFile offlineDocs(localDocs);
    if (offlineDocs.exists()) {
        return QUrl::fromUserInput(localDocs);
    }

    return QUrl::fromUserInput(QLatin1String(Ubuntu::Constants::UBUNTU_API_ONLINE));
}
