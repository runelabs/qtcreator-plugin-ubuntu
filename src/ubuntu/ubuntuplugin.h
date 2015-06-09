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

#ifndef UBUNTU_H
#define UBUNTU_H

#include "ubuntu_global.h"
#include "ubuntuwelcomemode.h"
#include "ubuntudevicemode.h"
#include "ubuntumenu.h"
#include "ubuntuprojectmanager.h"
#include "ubuntufeatureprovider.h"
#include "ubuntuversionmanager.h"
#include "ubuntupackagingmode.h"
#include "ubuntusettingsdeviceconnectivitypage.h"
#include "ubuntusettingsclickpage.h"

#include <extensionsystem/iplugin.h>

namespace Ubuntu {
namespace Internal {

class UBUNTUSHARED_EXPORT UbuntuPlugin: public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Ubuntu.json")

public:
    UbuntuPlugin();
    ~UbuntuPlugin();

    virtual bool initialize(const QStringList &arguments, QString *errorString) override;
    virtual void extensionsInitialized() override;

private slots:
    void onKitsLoaded ();
    void showFirstStartWizard ();
    void updateContextMenu(ProjectExplorer::Project *project,ProjectExplorer::Node *node);
    void migrateProject ();

protected:
    UbuntuDeviceMode       *m_ubuntuDeviceMode;
    UbuntuMenu             *m_ubuntuMenu;
    UbuntuPackagingMode    *m_ubuntuPackagingMode;
    QAction                *m_migrateProjectAction;

    ProjectExplorer::Project *m_currentContextMenuProject;
};


} // Internal
} // Ubuntu

#endif // UBUNTU_H

