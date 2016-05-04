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
#include "ubuntucreatenewchrootdialog.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/toolchainmanager.h>

#include <QMessageBox>

namespace Ubuntu {
namespace Internal {

bool UbuntuClickDialog::createClickChrootModal(bool redetectKits, const QString &arch, const QString &framework, QWidget *parent)
{

    UbuntuClickTool::Target t;
    if(!UbuntuCreateNewChrootDialog::getNewChrootTarget(&t,arch,framework,parent))
        return false;

    ProjectExplorer::ProcessParameters params;
    UbuntuClickTool::parametersForCreateChroot(t,&params);

    bool success = (runProcessModal(params,parent) == 0);

    if(success) {
        ClickToolChain* tc = new ClickToolChain(t, ProjectExplorer::ToolChain::AutoDetection);
        ProjectExplorer::ToolChainManager::registerToolChain(tc);

        if(redetectKits)
            UbuntuKitManager::autoDetectKits();
    }

    return success;
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
            QString title = tr(Constants::UBUNTU_CLICK_DELETE_TITLE);
            QString text  = tr(Constants::UBUNTU_CLICK_DELETE_MESSAGE);
            if( QMessageBox::question(Core::ICore::mainWindow(),title,text) != QMessageBox::Yes )
                return 0;
        }

        ProjectExplorer::ProcessParameters params;
        UbuntuClickTool::parametersForMaintainChroot(mode,target,&params);
        paramList<<params;
    }

    int code = runProcessModal(paramList);
    if (mode == UbuntuClickTool::Delete ) {
        UbuntuKitManager::autoDetectKits();
    }
    return code;
}

}}
