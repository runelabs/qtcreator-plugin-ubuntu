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

#ifndef UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
#define UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H

#include <QList>
#include <QString>
#include <QDialog>
#include <QFutureInterface>
#include <QQueue>
#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>
#include <QDebug>


class QDialogButtonBox;
class QPlainTextEdit;
class QLabel;
class QAction;

namespace ProjectExplorer {
    class Project;
    class Target;
}

namespace Ubuntu {
namespace Internal {

class UbuntuClickBuildConfiguration;

class UbuntuClickTool
{
public:

    enum MaintainMode {
        Upgrade,//runs click chroot upgrade
        Delete  //runs click chroot delete
    };

    struct Target {
        bool    maybeBroken;
        int     majorVersion;
        int     minorVersion;
        QString series;
        QString framework;
        QString architecture;
    };

    UbuntuClickTool();

    static void parametersForCreateChroot   (const Target &target, ProjectExplorer::ProcessParameters* params);
    static void parametersForMaintainChroot (const MaintainMode &mode,const Target& target,ProjectExplorer::ProcessParameters* params);

    static void openChrootTerminal (const Target& target);

    static QString targetBasePath (const Target& target);
    static bool getTargetFromUser (Target* target, const QString &framework=QString());
    static QStringList getSupportedFrameworks (const Target *target);
    static QString getMostRecentFramework ( const QString &subFramework, const Target *target );

    static bool          targetExists (const Target& target);
    static QList<Target> listAvailableTargets (const QString &framework=QString());
    static QPair<int,int> targetVersion (const Target& target);
    static bool        targetFromPath(const QString& targetPath, Target* tg);
    static const Target *clickTargetFromTarget(ProjectExplorer::Target *t);
};

QDebug operator<<(QDebug dbg, const UbuntuClickTool::Target& t);

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUCLICKTOOL_H
