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

#include "ubunturuncontrolfactory.h"

using namespace Ubuntu::Internal;

bool UbuntuRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                                ProjectExplorer::RunMode mode) const {
    if (!qobject_cast<UbuntuProject*>(runConfiguration->target()->project()))
        return false;
    if (mode == ProjectExplorer::NormalRunMode || mode == ProjectExplorer::DebugRunMode)
        return true;
    return false;
}

ProjectExplorer::RunControl *UbuntuRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration,
                                                        ProjectExplorer::RunMode mode, QString *) {
    QList<ProjectExplorer::RunControl *> runcontrols =
            ProjectExplorer::ProjectExplorerPlugin::instance()->runControls();
    foreach (ProjectExplorer::RunControl *rc, runcontrols) {
        if (UbuntuRunControl *qrc = qobject_cast<UbuntuRunControl *>(rc)) {
            qrc->stop();
        }
    }

    ProjectExplorer::RunControl *runControl = 0;
    if (mode == ProjectExplorer::NormalRunMode)
        runControl = new UbuntuRunControl(runConfiguration, mode, false);
    else if (mode == ProjectExplorer::DebugRunMode)
        runControl = new UbuntuRunControl(runConfiguration, mode, true);

    return runControl;
}

QString UbuntuRunControlFactory::displayName() const {
    return tr("Run on Ubuntu Touch Device");
}
