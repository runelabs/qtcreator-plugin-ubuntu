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

#ifndef UBUNTUCONSTANTS_H
#define UBUNTUCONSTANTS_H

#include <QString>
#include <coreplugin/icore.h>

namespace Ubuntu {
namespace Constants {

const char UBUNTUPROJECT_MIMETYPE[] = "application/x-ubuntuproject";
const char UBUNTUPROJECT_ID[] = "UbuntuProjectManager.UbuntuProject";
const char UBUNTUPROJECT_PROJECTCONTEXT[] = "UbuntuProject.ProjectContext";
const char UBUNTUPROJECT_SUFFIX[] = ".ubuntuproject";
const char UBUNTU_PROJECT_WIZARD_CATEGORY[] = "A.UbuntuProjects";
const char UBUNTUPROJECT_DISPLAYNAME[] = "Ubuntu Project";
const char UBUNTUPROJECT_RUNCONTROL_ID[] = "UbuntuProjectManager.UbuntuRunConfiguration";

const char UBUNTU_PROJECT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Ubuntu");

const char UBUNTU_UBUNTUPROJECT_TYPE[] = "ubuntuproject";
const char UBUNTU_HTMLPROJECT_TYPE[] = "htmlproject";
const char UBUNTU_AUTOPILOTPROJECT_TYPE[] = "autopilotproject";
const char UBUNTU_QMLPROJECT_TYPE[] = "qmlproject";
const char UBUNTU_CORDOVAUBUNTU_TYPE[] = "cordovaproject";
const char UBUNTU_QTPROJECT_TYPE[] = "pro";
const char UBUNTU_QML_TYPE[] = "qml";
const char UBUNTU_HAS_TESTS[] = "hasTests";

const QString UBUNTU_WELCOMESCREEN_QML = Core::ICore::resourcePath() + QLatin1String("/ubuntu/welcome/welcome.qml");
const QString UBUNTU_TEMPLATESPATH = Core::ICore::resourcePath() + QLatin1String("/templates/wizards/ubuntu/");
const QString UBUNTU_MENUPATH = Core::ICore::resourcePath() + QLatin1String("/ubuntu/");
const QString UBUNTU_SHAREPATH = Core::ICore::resourcePath() + QLatin1String("/ubuntu/");
const QString UBUNTU_SCRIPTPATH = Core::ICore::resourcePath() + QLatin1String("/ubuntu/scripts");

const char UBUNTU_MENUJSON[] = "menu.json";
const char UBUNTU_MENUJSON_NAME[] = "name";
const char UBUNTU_MENUJSON_ID[] = "id";
const char UBUNTU_MENUJSON_ACTIONS[] = "actions";
const char UBUNTU_MENUJSON_SUBMENU[] = "submenu";
const char UBUNTU_MENUJSON_KEYSEQUENCE[] = "keysequence";
const char UBUNTU_MENUJSON_QUERYDIALOG[] = "queryDialog";
const char UBUNTU_MENUJSON_TITLE[] = "title";
const char UBUNTU_MENUJSON_MESSAGE[] = "message";
const char UBUNTU_MENUJSON_VALUE[] = "value";
const char UBUNTU_MENUJSON_PARENT[] = "parent";
const char UBUNTU_MENUJSON_GROUP[] = "group";
const char UBUNTU_MENUJSON_WORKINGDIRECTORY[] = "workingDirectory";
const char UBUNTU_MENUJSON_PROJECTREQUIRED[] = "projectRequired";
const char UBUNTU_MENUJSON_DEVICEREQUIRED[] = "deviceRequired";
const char UBUNTU_MENUJSON_QMLPROJECTREQUIRED[] = "qmlProjectRequired";
const char UBUNTU_MENUJSON_CORDOVAPROJECTREQUIRED[] = "cordovaProjectRequired";
const char UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED[] = "qmakeProjectRequired";
const char UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED[] = "ubuntuProjectRequired";
const char UBUNTU_MENUJSON_SAVEREQUIRED[] = "saveRequired";
const char UBUNTU_MENUJSON_MESSAGEDIALOG[] = "messageDialog";

const char UBUNTU_MENUJSON_PARENT_TOOLS[] = "Tools";
const char UBUNTU_MENUJSON_PARENT_WINDOW[] = "Window";
const char UBUNTU_MENUJSON_PARENT_HELP[] = "Help";
const char UBUNTU_MENUJSON_PARENT_BUILD[] = "Build";
const char UBUNTU_MENUJSON_PARENT_FILE[] = "File";
const char UBUNTU_MENUJSON_PARENT_EDIT[] = "Edit";
const char UBUNTU_MENUJSON_PARENT_TOP[] = "TOP";

const char UBUNTU_PROJECTJSON[] = "projectypes.json";
const char UBUNTU_PROJECTJSON_DISPLAYNAME[] = "displayName";
const char UBUNTU_PROJECTJSON_ID[] = "id";
const char UBUNTU_PROJECTJSON_DESCRIPTION[] = "description";
const char UBUNTU_PROJECTJSON_FOLDER[] = "folder";
const char UBUNTU_PROJECTJSON_TYPE[] = "type";
const char UBUNTU_PROJECTJSON_MAINFILE[] = "mainFile";
const char UBUNTU_PROJECTJSON_PROJECTFILE[] = "projectFile";
const char UBUNTU_PROJECTJSON_FILENAME[] = "fileName";
const char UBUNTU_PROJECTJSON_FILES[] = "files";
const char UBUNTU_PROJECTJSON_CATEGORY_DISPLAY[] = "categoryDisplay";
const char UBUNTU_PROJECTJSON_CATEGORY[] = "category";
const char UBUNTU_PROJECTJSON_REQUIRED_FEATURE[] = "requiredFeature";

const char UBUNTU_DIALOG_NO_PROJECTOPEN_TITLE[] = "No project open";
const char UBUNTU_DIALOG_NO_PROJECTOPEN_TEXT[] = "Open a project or create a new one.";

const char UBUNTU_PROCESS_COMMAND[] = "command";

const char UBUNTU_ACTION_FOLDERNAME[] = "%FOLDERNAME%";
const char UBUNTU_ACTION_PROJECTDIRECTORY[] = "%PROJECTDIRECTORY%";
const char UBUNTU_ACTION_DISPLAYNAME[] = "%DISPLAYNAME%";
const char UBUNTU_ACTION_DISPLAYNAME_UPPER[] = "%DISPLAYNAME_UPPER%";
const char UBUNTU_ACTION_DISPLAYNAME_LOWER[] = "%DISPLAYNAME_LOWER%";
const char UBUNTU_ACTION_DISPLAYNAME_CAPITAL[] = "%DISPLAYNAME_CAPITAL%";
const char UBUNTU_ACTION_PROJECTFILES[] = "%PROJECTFILES%";
const char UBUNTU_ACTION_SCRIPTDIRECTORY[] = "%SCRIPTDIRECTORY%";
const char UBUNTU_ACTION_SHAREDIRECTORY[] = "%SHAREDIRECTORY%";
const char UBUNTU_ACTION_SERIALNUMBER[] = "%SERIALNUMBER%";

const char UBUNTU_FILENAME_DISPLAYNAME[] = "displayName";
const char UBUNTU_FILENAME_DISPLAYNAME_LOWER[] = "displayName_lower";
const char UBUNTU_FILENAME_DISPLAYNAME_UPPER[] = "displayName_upper";
const char UBUNTU_FILENAME_DISPLAYNAME_CAPITAL[] = "displayName_capital";

const char UBUNTU_MODE_WELCOME[] = "UbuntuWelcome";
const char UBUNTU_MODE_WELCOME_DISPLAYNAME[] = "Touch";
const char UBUNTU_MODE_WELCOME_ICON[] = ":/ubuntu/images/ubuntu-qtcreator.png";
const int  UBUNTU_MODE_WELCOME_PRIORITY = 1;

const char UBUNTU_MODE_PACKAGING[] = "UbuntuPackaging";
const char UBUNTU_MODE_PACKAGING_DISPLAYNAME[] = "Packaging";
const char UBUNTU_MODE_PACKAGING_ICON[] = ":/ubuntu/images/packaging.png";
const int  UBUNTU_MODE_PACKAGING_PRIORITY = 80;

const char UBUNTU_MODE_DEVICES[] = "UbuntuDevices";
const char UBUNTU_MODE_DEVICES_DISPLAYNAME[] = "Devices";
const char UBUNTU_MODE_DEVICES_ICON[] = ":/ubuntu/images/device.png";
const int  UBUNTU_MODE_DEVICES_PRIORITY = 11;

const char UBUNTU_MODE_WEB[] = "UbuntuWeb";
const char UBUNTU_MODE_WEB_DISPLAYNAME[] = "WEB";
const char UBUNTU_MODE_WEB_ICON[] = ":/ubuntu/images/ubuntu-32.png";
const int  UBUNTU_MODE_WEB_PRIORITY = 10;

const char UBUNTU_MODE_PASTEBIN[] = "UbuntuPasteBin";
const char UBUNTU_MODE_PASTEBIN_DISPLAYNAME[] = "Pastebin";

const char UBUNTU_MODE_IRC[] = "UbuntuIRC";
const char UBUNTU_MODE_IRC_DISPLAYNAME[] = "IRC";

const char UBUNTU_MODE_API[] = "UbuntuAPI";
const char UBUNTU_MODE_API_DISPLAYNAME[] = "API";

const char UBUNTU_MODE_COREAPPS[] = "UbuntuCoreApps";
const char UBUNTU_MODE_COREAPPS_DISPLAYNAME[] = "Core Apps";

const char UBUNTU_MODE_WIKI[] = "UbuntuWiki";
const char UBUNTU_MODE_WIKI_DISPLAYNAME[] = "Wiki";

const char UBUNTU_IRC[] = "http://webchat.freenode.net/?channels=ubuntu-app-devel";
const char UBUNTU_API_ONLINE[] = "http://developer.ubuntu.com/api/devel/ubuntu-13.10/qml/ui-toolkit/overview-ubuntu-sdk.html";
const char UBUNTU_API_OFFLINE[] = "/usr/share/ubuntu-ui-toolkit/doc/html/overview-ubuntu-sdk.html";
const char UBUNTU_COREAPPS[] = "https://launchpad.net/ubuntu-phone-coreapps/";
const char UBUNTU_WIKI[] = "https://wiki.ubuntu.com/Touch";
const char UBUNTU_PASTEBIN[] = "http://pastebin.ubuntu.com";

const char FEATURE_UNITY_SCOPE[] = "Ubuntu.Wizards.FeatureUnityScope";
const char FEATURE_UBUNTU_PRECISE[] = "Ubuntu.Wizards.FeatureUbuntuPrecise";
const char FEATURE_UBUNTU_QUANTAL[] = "Ubuntu.Wizards.FeatureUbuntuQuantal";
const char FEATURE_UBUNTU_RARING[] = "Ubuntu.Wizards.FeatureUbuntuRaring";
const char FEATURE_UBUNTU_SAUCY[] = "Ubuntu.Wizards.FeatureUbuntuSaucy";

const char DISTRIB_ID[] = "DISTRIB_ID=";
const char DISTRIB_CODENAME[] = "DISTRIB_CODENAME=";
const char DISTRIB_RELEASE[] = "DISTRIB_RELEASE=";
const char DISTRIB_DESCRIPTION[] = "DISTRIB_DESCRIPTION=";
const char LSB_RELEASE[] = "/etc/lsb-release";

const char PRECISE[] = "precise";
const char QUANTAL[] = "quantal";
const char RARING[] = "raring";
const char SAUCY[] = "saucy";

const char PLATFORM_DESKTOP[] = "Desktop";
const char PLATFORM_DESKTOP_DISPLAYNAME[] = "Ubuntu %0";

const char TASK_DEVICE_SCRIPT[] = "Ubuntu.Task.DeviceScript";

const char UBUNTU_SETTINGS_ICON[] = ":/ubuntu/images/ubuntu-32.png";

} // namespace Ubuntu
} // namespace Constants

#endif // UBUNTUCONSTANTS_H

