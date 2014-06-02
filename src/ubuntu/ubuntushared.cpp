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
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#include "ubuntushared.h"

#include <utils/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

bool readFile(const QString &fileName, QByteArray *data, QString *errorMessage)  {
    Utils::FileReader reader;
    if (!reader.fetch(fileName, errorMessage)) return false;
    *data = reader.data();
    return true;
}

void printToOutputPane(const QString &msg) {
    QString timestamp = QDateTime::currentDateTime().toString(QString::fromLatin1("HH:mm:ss"));
    Core::MessageManager::write(QString(QLatin1String("[%0] %1")).arg(timestamp).arg(msg),Core::MessageManager::NoModeSwitch);
}
