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
 */

#ifndef CORDOVAUBUNTUPROJECTMANAGER_H
#define CORDOVAUBUNTUPROJECTMANAGER_H

#include "global.h"
#include "constants.h"
#include "common.h"

#include <ubuntu/ubuntuprocess.h>

namespace CordovaUbuntuProjectManager {

class CORDOVAUBUNTUPROJECTMANAGERSHARED_EXPORT CordovaUbuntuProjectManagerPlugin : public ExtensionSystem::IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CordovaUbuntuProjectManager.json")
    
public:
    CordovaUbuntuProjectManagerPlugin() {}
    ~CordovaUbuntuProjectManagerPlugin() {}
    
    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized() {}

private slots:
    void menuItemTriggered(void);
    void onStarted(QString);
    void onMessage(QString);
    void onError(QString);
    void onFinished(QString cmd, int code);
    void slotUpdateActions();

private:
    Ubuntu::Internal::UbuntuProcess m_ubuntuProcess;
    QAction* m_actionMenu;
};

}

#endif

