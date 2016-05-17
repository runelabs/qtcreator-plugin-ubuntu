/*
 * Copyright 2016 Canonical Ltd.
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
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#include "ubuntuclickdialog.h"
#include "ubuntuconstants.h"
#include "clicktoolchain.h"
#include "ubuntukitmanager.h"
#include <ubuntu/wizards/createtargetwizard.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitmanager.h>
#include <coreplugin/helpmanager.h>
#include <qtsupport/qtversionmanager.h>

#include <QMessageBox>

namespace Ubuntu {
namespace Internal {

bool UbuntuClickDialog::doCreateTarget (bool redetectKits, const UbuntuClickTool::Target &t, QWidget *parent)
{
    ProjectExplorer::ProcessParameters params;
    UbuntuClickTool::parametersForCreateChroot(t, &params);

    bool success = (runProcessModal(params, parent) == 0);

    if(success) {
        ClickToolChain* tc = new ClickToolChain(t, ProjectExplorer::ToolChain::AutoDetection);
        ProjectExplorer::ToolChainManager::registerToolChain(tc);

        if(redetectKits)
            UbuntuKitManager::autoDetectKits();
    }

    return success;
}

bool UbuntuClickDialog::createClickChrootModal(bool redetectKits, QWidget *parent)
{
    UbuntuClickTool::Target t;
    if(!CreateTargetWizard::getNewTarget(&t, parent))
        return false;
    return doCreateTarget(redetectKits, t, parent);
}

bool UbuntuClickDialog::createClickChrootModal(bool redetectKits, const QString &arch, const QString &framework, QWidget *parent)
{

    UbuntuClickTool::Target t;
    if(!CreateTargetWizard::getNewTarget(&t,arch,framework,parent))
        return false;
    return doCreateTarget(redetectKits, t, parent);
}

int UbuntuClickDialog::maintainClickModal(const UbuntuClickTool::Target &target, const UbuntuClickTool::MaintainMode &mode)
{
    return maintainClickModal(QList<UbuntuClickTool::Target>()<<target,mode);
}

int UbuntuClickDialog::maintainClickModal(const QList<UbuntuClickTool::Target> &targetList, const UbuntuClickTool::MaintainMode &mode)
{
    QList<ProjectExplorer::ProcessParameters> paramList;
    foreach(const UbuntuClickTool::Target &target, targetList) {
        if(mode == UbuntuClickTool::Delete) {

            QStringList docToRemove;
            QString rootfs = UbuntuClickTool::targetBasePath(target);
            if (!rootfs.isEmpty()) {
                foreach(const QString &ns, Core::HelpManager::registeredNamespaces()) {
                    QString fileName = Core::HelpManager::fileFromNamespace(ns);
                    if(fileName.startsWith(rootfs)) {
                        docToRemove.append(ns);
                    }
                }
            }

            QString title = tr(Constants::UBUNTU_CLICK_DELETE_TITLE);
            QString text  = tr(Constants::UBUNTU_CLICK_DELETE_MESSAGE);
            if( QMessageBox::question(Core::ICore::mainWindow(),title,text) != QMessageBox::Yes )
                return 0;

            //remove all kits using the target
            QList<ProjectExplorer::Kit *> kitsToDelete = UbuntuKitManager::findKitsUsingTarget(target);
            foreach(ProjectExplorer::Kit *curr, kitsToDelete) {
                ProjectExplorer::KitManager::deregisterKit(curr);
            }

            //make sure no help files are still opened
            Core::HelpManager::unregisterDocumentation(docToRemove);
        }

        ProjectExplorer::ProcessParameters params;
        UbuntuClickTool::parametersForMaintainChroot(mode,target,&params);
        paramList<<params;
    }

    int code = runProcessModal(paramList);
    if(mode == UbuntuClickTool::Delete) {
        //redetect documentation
        QtSupport::QtVersionManager::delayedInitialize();
    }
    return code;
}

}}
