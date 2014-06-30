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

#ifndef UBUNTUBZR_H
#define UBUNTUBZR_H

#include <QObject>
#include <QProcess>

namespace Ubuntu {
namespace Internal {

class UbuntuBzr : public QObject
{
    Q_OBJECT

public:
    explicit UbuntuBzr();
    ~UbuntuBzr();

    static UbuntuBzr *instance ();

signals:
    void initializedChanged();

public slots:
    bool isInitialized() { return m_bInitialized; }
    QString whoami() { return m_whoami; }
    QString launchpadId() { return m_launchpadId; }
    void initialize();
    bool waitForFinished(int msecs = 30000);

protected slots:
    void scriptExecuted(int);

protected:
    static UbuntuBzr *m_instance;

    bool m_bInitialized;
    QString m_whoami;
    QString m_launchpadId;

    QProcess m_cmd;
};

}
}
#endif // UBUNTUBZR_H
