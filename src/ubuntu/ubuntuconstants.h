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

#ifndef UBUNTUCONSTANTS2_H
#define UBUNTUCONSTANTS2_H

#include <QString>
#include <coreplugin/icore.h>


namespace Ubuntu {
namespace Constants {

const char LINEFEED[] = "\n";
const char UNDERLINE[] = "_";
const char DASH[] = "-";
const char EMPTY[] = "";
const char SPACE[] = " ";
const char TAB[] = "	";
const char INSTALLED[] = "ii";
const char ZERO_STR[] = "0";
const char ONE_STR[] = "1";
const char USERNAME[] = "username";

const char UBUNTU_MIMETYPE_XML[] = ":/ubuntu/UbuntuProject.mimetypes.xml";
const char UBUNTUDEVICESWIDGET_ONERROR[] = "<p style=\"color: red\">%0</p>";
const char UBUNTUDEVICESWIDGET_ACTION_BEGIN[] = "<p style=\"color: #888\">%0</p>";
const char UBUNTUDEVICESWIDGET_ACTION_END[] = "<p style=\"color: #888\">%0</p>";
const char UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED[] = "The package is not installed.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_ADB_REGEX[] = "(\\S+)\\s+(.*)";
const char UBUNTUDEVICESWIDGET_ONFINISHED_ADB_NOACCESS[] = "???";
const char UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_STARTED[] = "..openssh-server was started.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_NONE[] = "(none)";
const char UBUNTUDEVICESWIDGET_ONFINISHED_SSH_IS_INSTALLED[] = "..openssh-server (%0) is installed.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_SSH_NOT_INSTALLED[] = "..openssh-server was not installed.";

const char UBUNTUDEVICESWIDGET_ONFINISHED_UNABLE_TO_FETCH[] = "E: Unable to fetch some archives, maybe run apt-get update or try with --fix-missing?";

const char UBUNTUWIDGETS_ONFINISHED_SCRIPT_LOCAL_PACKAGE_INSTALLED[] = "%0/local_package_installed";  
const char UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_LOCAL_INSTALL_EMULATOR[] = "%0/local_install_emulator";

const char UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_ENABLED[] = "..writable image has been enabled, device is rebooting.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLE_DISABLED[] = "..writable image has been disabled, device is rebooting.";

const char UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_REMOVED[] = "..developer tools have been removed.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_NOT_INSTALLED[] = "..developer tools are not installed.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_INSTALLED[] = "..developer tools are already installed.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_WRITABLEIMAGE[] = "..writable image.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_READONLYIMAGE[] = "..read-only image.";

const char UBUNTUDEVICESWIDGET_ONFINISHED_DEVELOPERTOOLS_WAS_INSTALLED[] = "..platform development was enabled.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_REMOVED[] = "..openssh-server was removed.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_SSH_INSTALL[] = "%0/openssh_install";
const char UBUNTUDEVICESWIDGET_ONFINISHED_SSH_WAS_INSTALLED[] = "..openssh-server was installed.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_SCRIPT_PORTFORWARD[] = "%0/device_portforward";
const char UBUNTUDEVICESWIDGET_ONFINISHED_PORTS_FORWARDED[] = "..ports forwarded.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_PUBLICKEY_AUTH_SET[] = "..public key authentication is now set.";
const char UBUNTUDEVICESWIDGET_ONFINISHED_NETWORK_CONF_COPIED[] = "..network configuration copied.";

const char EMULATOR_PACKAGE_NAME[] = "ubuntu-emulator";
const char DEFAULT_EMULATOR_PATH[] = "ubuntu-emulator";
const char REVIEWER_PACKAGE_NAME[] = "click-reviewers-tools";

const char UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED[] = "Checking installed emulator package.";
const char UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE[] = "Install the emulator package on the system..";
const char UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE_SCRIPT[] = "%0/local_install_emulator %1";
const char UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES_SCRIPT[] = "%0/local_search_images";
const char UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES[] = "Search configured emulator instances.";
const char UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR[] = "Creating new emulator instance.";
const char UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR_SCRIPT[] = "%0 %1/local_create_emulator %2 %3 %4 %5 %6 %7 %8";
const char UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR[] = "Starting the selected emulator.";
const char UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR_SCRIPT[] = "%0/local_start_emulator";
const char UBUNTUDEVICESWIDGET_LOCAL_STOP_EMULATOR_SCRIPT[] = "%0/local_stop_emulator";
const char UBUNTUDEVICESWIDGET_LOCAL_DELETE_EMULATOR_SCRIPT[] = "%0/local_delete_emulator";

const char UBUNTUDEVICESWIDGET_STARTSSHSERVICE[] = "Start ssh service on device..";
const char UBUNTUDEVICESWIDGET_STARTSSHSERVICE_SCRIPT[] = "%0/device_service_ssh_start %1";

const char UBUNTUDEVICESWIDGET_MAKEFSWRITABLE[] = "Make filesystem writable..";
const char UBUNTUDEVICESWIDGET_MAKEFSWRITABLE_SCRIPT[] = "%0/device_writableimage_set %1";
const char UBUNTUDEVICESWIDGET_MAKEFSREADONLY[] = "Make filesystem read-only..";
const char UBUNTUDEVICESWIDGET_MAKEFSREADONLY_SCRIPT[] = "%0/device_writableimage_unset %1";
const char UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS[] = "Are developer tools installed..";
const char UBUNTUDEVICESWIDGET_DETECTDEVELOPERTOOLS_SCRIPT[] = "%0/device_developertools_has %1";
const char UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE[] = "Is device image read-only or writable..";
const char UBUNTUDEVICESWIDGET_DETECTWRITABLEIMAGE_SCRIPT[] =  "%0/device_writableimage_has %1";
const char UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT[] = "Disable Platform Development..";
const char UBUNTUDEVICESWIDGET_DISABLEPLATFORMDEVELOPMENT_SCRIPT[] = "%0/device_developertools_remove %1";
const char UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT[] = "Enable Platform Development..";
const char UBUNTUDEVICESWIDGET_ENABLEPLATFORMDEVELOPMENT_SCRIPT[] = "%0/device_developertools_install %1";

const char UBUNTUPROJECT_MIMETYPE[] = "application/x-ubuntuproject";
const char UBUNTUPROJECT_ID[] = "UbuntuProjectManager.UbuntuProject";
const char UBUNTUPROJECT_PROJECTCONTEXT[] = "UbuntuProject.ProjectContext";
const char UBUNTUPROJECT_SUFFIX[] = ".ubuntuproject";
const char UBUNTUHTMLPROJECT_SUFFIX[] = ".ubuntuhtmlproject";
const char UBUNTUPROJECT_RUNCONTROL_BASE_ID[] = "UbuntuProjectManager.UbuntuRunConfiguration.Container";
const char UBUNTUPROJECT_RUNCONTROL_SCOPE_ID[] = "UbuntuProjectManager.UbuntuRunConfiguration.Container.Scope";
const char UBUNTUPROJECT_RUNCONTROL_APP_ID[] = "UbuntuProjectManager.UbuntuRunConfiguration.Container.App";
const char UBUNTUPROJECT_REMOTE_RUNCONTROL_BASE_ID[] = "UbuntuProjectManager.RemoteRunConfiguration";
const char UBUNTUPROJECT_REMOTE_RUNCONTROL_SCOPE_ID[] = "UbuntuProjectManager.RemoteRunConfiguration.Scope";
const char UBUNTUPROJECT_REMOTE_RUNCONTROL_APP_ID[] = "UbuntuProjectManager.RemoteRunConfiguration.App";

const char UBUNTUHTML_PROJECT_LAUNCHER_EXE[] = "/usr/bin/ubuntu-html5-app-launcher";
const char UBUNTUWEBAPP_PROJECT_LAUNCHER_EXE[] = "/usr/bin/webapp-container";
const char UBUNTUSCOPES_PROJECT_LAUNCHER_EXE[] = "qtc_desktop_scoperunner.py";

const char UBUNTUBZR_INITIALIZE[] ="%0/qtc_bzr_info";
const char UBUNTUPACKAGINGWIDGET_MENU_REMOVE[] ="Remove";
const char UBUNTUPACKAGINGWIDGET_BUILDPACKAGE_ID[] ="Ubuntu.Build.Package";
const char UBUNTUPACKAGINGWIDGET_DEFAULT_MANIFEST[] =":/ubuntu/manifest.json.template";
const char UBUNTUPACKAGINGWIDGET_DEFAULT_MYAPP[] =":/ubuntu/myapp.json.template";
const char UBUNTUPACKAGINGWIDGET_DEFAULT_NAME[] ="%0.%1";
const char UBUNTUPACKAGINGWIDGET_LOCAL_REVIEWER_INSTALLED[] = "Checking installed click reviewer tools package.";
const char UBUNTUPACKAGINGWIDGET_CLICK_REVIEWER_TOOLS_AGAINST_PACKAGE[] = "Click Reviewers tools against %0";
const char UBUNTUPACKAGINGWIDGET_CLICK_DEPLOY_SCRIPT[] = "%1/qtc_project_click_deploy %2 %3 %4 %5 %6 %7";
const char UBUNTUPACKAGINGWIDGET_CLICK_DEPLOY_MESSAGE[] = "Installing click package on device";
const char UBUNTU_CLICK_PACKAGE_MASK[] = "*.click";
const char UBUNTU_CLICK_PACKAGE_SELECTOR_TEXT[] = "Select click package which you want to test";

const char QMAKE_MIMETYPE[] = "application/vnd.qt.qmakeprofile";
const char QMLPROJECT_MIMETYPE[] = "application/x-qmlproject";
const char UBUNTUMENU_ONFINISHED[] = "%0 finished with code %1";
const char UBUNTUMENU_ONERROR[] = "%0";
const char UBUNTUMENU_ONSTARTED[] = "Started %0";
const char ERROR_MSG_NOACTIONS[] = "No actions defined in map";
const char ERROR_MSG_NO_MENU_DEFINED[] = "No menu defined";
const char ERROR_MSG_COULD_NOT_CAST_TO_ACTION[] = "Could not cast to action";
const char ERROR_MSG_UNABLE_TO_PARSE_MENUJSON[] = "Unable to parse menu.json";

const char UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT[] = "%0/local_package_installed %1";
const char UBUNTUDEVICESWIDGET_DETECTOPENSSH[] = "Detecting if openssh-server is installed..";
const char UBUNTUDEVICESWIDGET_DETECTOPENSSH_SCRIPT[] = "%0/openssh_version %1";
const char UBUNTUDEVICESWIDGET_DETECTDEVICES[] = "Detecting device..";
const char UBUNTUDEVICESWIDGET_DETECTDEVICES_SCRIPT[] = "%1/device_search %2";
const char UBUNTUDEVICESWIDGET_SSHCONNECT_SCRIPT[] = "%0/openssh_connect";

const char UBUNTUDEVICESWIDGET_CLONENETWORK[] = "Clone network configuration from host to device..";
const char UBUNTUDEVICESWIDGET_CLONENETWORK_SCRIPT[] = "%0/device_network_clone %1";
const char UBUNTUDEVICESWIDGET_PORTFORWARD[] = "Enabling port forward..";
const char UBUNTUDEVICESWIDGET_PORTFORWARD_SCRIPT[] = "%0/device_portforward";
const char UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH[] = "Setting up public key authentication..";
const char UBUNTUDEVICESWIDGET_SETUP_PUBKEY_AUTH_SCRIPT[] = "%0/device_publickey_deploy %1 %2";
const char UBUNTUDEVICESWIDGET_HASNETWORK[] = "Check if the device is connected to a network..";
const char UBUNTUDEVICESWIDGET_HASNETWORK_SCRIPT[] = "%0/device_hasnetwork %1";
const char UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION[] = "Check device image version..";
const char UBUNTUDEVICESWIDGET_WAIT_FOR_BOOT_MESSAGE[] = "Waiting for device to come up..";
const char UBUNTUDEVICESWIDGET_WAIT_FOR_BOOT_SCRIPT[] = "%0/device_wait_for_shell %1";
const char UBUNTUDEVICESWIDGET_WAIT_FOR_EMULATOR_MESSAGE[] = "Waiting for emulator tool to come up..";
const char UBUNTUDEVICESWIDGET_WAIT_FOR_EMULATOR_SCRIPT[] = "%1/local_wait_for_emulator %2";
const char UBUNTUDEVICESWIDGET_DETECTDEVICEVERSION_SCRIPT[] = "%0/device_version %1";
const char UBUNTUDEVICESWIDGET_SSH_INSTALL[] = "Installing openssh-server..";
const char UBUNTUDEVICESWIDGET_SSH_INSTALL_SCRIPT[] = "%0/openssh_install %1";
const char UBUNTUDEVICESWIDGET_SSH_REMOVE[] = "Removing openssh-server..";
const char UBUNTUDEVICESWIDGET_SSH_REMOVE_SCRIPT[] = "%0/openssh_remove %1";

#ifdef UBUNTU_BUILD_LOCAL
const QString UBUNTU_RESOURCE_PATH = QLatin1String(UBUNTU_RESOURCE_PATH_LOCAL);
#else
const QString UBUNTU_RESOURCE_PATH = Core::ICore::resourcePath();
#endif

const QString UBUNTU_WELCOMESCREEN_QML = UBUNTU_RESOURCE_PATH + QLatin1String("/ubuntu/qml/welcome.qml");
const QString UBUNTU_DEVICESCREEN_QML  = UBUNTU_RESOURCE_PATH + QLatin1String("/ubuntu/qml/devicespage.qml");
const QString UBUNTU_PUBLISHSCREEN_QML  = UBUNTU_RESOURCE_PATH + QLatin1String("/ubuntu/qml/publishpage.qml");
const QString UBUNTU_DEVICESCREEN_ROOT  = UBUNTU_RESOURCE_PATH + QLatin1String("/ubuntu/qml");
const QString UBUNTU_MENUPATH = UBUNTU_RESOURCE_PATH + QLatin1String("/ubuntu/");
const QString UBUNTU_SHAREPATH = UBUNTU_RESOURCE_PATH + QLatin1String("/ubuntu/");
const QString UBUNTU_SCRIPTPATH = UBUNTU_RESOURCE_PATH + QLatin1String("/ubuntu/scripts");

const char  UBUNTU_MENUJSON[] = "menu.json";
const char  UBUNTU_MENUJSON_NAME[] = "name";
const char  UBUNTU_MENUJSON_ID[] = "id";
const char  UBUNTU_MENUJSON_ACTIONS[] = "actions";
const char  UBUNTU_MENUJSON_SUBMENU[] = "submenu";
const char  UBUNTU_MENUJSON_KEYSEQUENCE[] = "keysequence";
const char  UBUNTU_MENUJSON_QUERYDIALOG[] = "queryDialog";
const char  UBUNTU_MENUJSON_TITLE[] = "title";
const char  UBUNTU_MENUJSON_MESSAGE[] = "message";
const char  UBUNTU_MENUJSON_VALUE[] = "value";
const char  UBUNTU_MENUJSON_PARENT[] = "parent";
const char  UBUNTU_MENUJSON_GROUP[] = "group";
const char  UBUNTU_MENUJSON_WORKINGDIRECTORY[] = "workingDirectory";
const char  UBUNTU_MENUJSON_PROJECTREQUIRED[] = "projectRequired";
const char  UBUNTU_MENUJSON_DEVICEREQUIRED[] = "deviceRequired";
const char  UBUNTU_MENUJSON_QMLPROJECTREQUIRED[] = "qmlProjectRequired";
const char  UBUNTU_MENUJSON_QMAKEPROJECTREQUIRED[] = "qmakeProjectRequired";
const char  UBUNTU_MENUJSON_UBUNTUPROJECTREQUIRED[] = "ubuntuProjectRequired";
const char  UBUNTU_MENUJSON_UBUNTUHTMLPROJECTREQUIRED[] = "ubuntuHtmlProjectRequired";
const char  UBUNTU_MENUJSON_CLICKTARGETREQUIRED[] = "needsClickTarget"; //will ask the user to choose a click target
const char  UBUNTU_MENUJSON_CLICKTOOLCHAINREQUIRED[] = "needsClickToolchain"; //requires a click toolchain
const char  UBUNTU_MENUJSON_SAVEREQUIRED[] = "saveRequired";
const char  UBUNTU_MENUJSON_MESSAGEDIALOG[] = "messageDialog";
const char  UBUNTU_MENUJSON_METACALL[] = "metacall";
const char  UBUNTU_MENUJSON_METACALL_ARGS[] = "args";
const char  UBUNTU_MENUJSON_METACALL_METHOD[] = "method";
const char  UBUNTU_MENUJSON_CONTEXT[] = "context";

const char  UBUNTU_MENUJSON_PARENT_TOOLS[] = "Tools";
const char  UBUNTU_MENUJSON_PARENT_WINDOW[] = "Window";
const char  UBUNTU_MENUJSON_PARENT_HELP[] = "Help";
const char  UBUNTU_MENUJSON_PARENT_BUILD[] = "Build";
const char  UBUNTU_MENUJSON_PARENT_FILE[] = "File";
const char  UBUNTU_MENUJSON_PARENT_EDIT[] = "Edit";
const char  UBUNTU_MENUJSON_PARENT_TOP[] = "TOP";

const char  UBUNTU_DIALOG_NO_PROJECTOPEN_TITLE[] = "No project open";
const char  UBUNTU_DIALOG_NO_PROJECTOPEN_TEXT[] = "Open a project or create a new one.";

const char  UBUNTU_ACTION_FOLDERNAME[] = "%FOLDERNAME%";
const char  UBUNTU_ACTION_PROJECTDIRECTORY[] = "%PROJECTDIRECTORY%";
const char  UBUNTU_ACTION_DISPLAYNAME[] = "%DISPLAYNAME%";
const char  UBUNTU_ACTION_PROJECTFILES[] = "%PROJECTFILES%";
const char  UBUNTU_ACTION_NAME_FROM_MANIFEST[] = "%NAME_FROM_MANIFEST%";
const char  UBUNTU_ACTION_SCRIPTDIRECTORY[] = "%SCRIPTDIRECTORY%";
const char  UBUNTU_ACTION_SHAREDIRECTORY[] = "%SHAREDIRECTORY%";
const char  UBUNTU_ACTION_SERIALNUMBER[] = "%SERIALNUMBER%";
const char  UBUNTU_ACTION_DEVICE_USERNAME[] = "%USERNAME%";
const char  UBUNTU_ACTION_DEVICE_IP[] = "%IP%";
const char  UBUNTU_ACTION_DEVICE_PORT[] = "%PORT%";
const char  UBUNTU_ACTION_APP_RUNNER_EXECNAME[] = "%APPRUNNEREXECNAME%";
const char  UBUNTU_ACTION_CLICK_ARCH[] = "%CLICK_ARCH%";
const char  UBUNTU_ACTION_CLICK_FRAMEWORK[] = "%CLICK_FRAMEWORK%";
const char  UBUNTU_ACTION_CLICK_PACKAGING_FOLDER[] = "%CLICK_PACKAGING_FOLDER%";

const char  UBUNTU_MODE_PACKAGING[] = "UbuntuPackaging";
const char  UBUNTU_MODE_PACKAGING_DISPLAYNAME[] = "Publish";
const char  UBUNTU_MODE_PACKAGING_ICON[] = ":/ubuntu/images/packaging.png";
const int   UBUNTU_MODE_PACKAGING_PRIORITY = 80;

const char  UBUNTU_MODE_DEVICES[] = "UbuntuDevices";
const char  UBUNTU_MODE_DEVICES_DISPLAYNAME[] = "Devices";
const char  UBUNTU_MODE_DEVICES_ICON[] = ":/ubuntu/images/device.png";
const int   UBUNTU_MODE_DEVICES_PRIORITY = 11;

const char  UBUNTU_ICON[] = ":/ubuntu/images/ubuntu-32.png";

const char  FEATURE_UNITY_SCOPE[] = "Ubuntu.Wizards.FeatureUnityScope";
const char  FEATURE_UBUNTU_PRECISE[] = "Ubuntu.Wizards.FeatureUbuntuPrecise";
const char  FEATURE_UBUNTU_QUANTAL[] = "Ubuntu.Wizards.FeatureUbuntuQuantal";
const char  FEATURE_UBUNTU_RARING[] = "Ubuntu.Wizards.FeatureUbuntuRaring";
const char  FEATURE_UBUNTU_SAUCY[] = "Ubuntu.Wizards.FeatureUbuntuSaucy";
const char  FEATURE_UBUNTU_TRUSTY[] = "Ubuntu.Wizards.FeatureUbuntuTrusty";
const char  FEATURE_UBUNTU_UTOPIC[] = "Ubuntu.Wizards.FeatureUbuntuUtopic";

const char  DISTRIB_ID[] = "DISTRIB_ID=";
const char  DISTRIB_CODENAME[] = "DISTRIB_CODENAME=";
const char  DISTRIB_RELEASE[] = "DISTRIB_RELEASE=";
const char  DISTRIB_DESCRIPTION[] = "DISTRIB_DESCRIPTION=";
const char  LSB_RELEASE[] = "/etc/lsb-release";

const char  PRECISE[] = "precise";
const char  QUANTAL[] = "quantal";
const char  RARING[] = "raring";
const char  SAUCY[] = "saucy";
const char  TRUSTY[] = "trusty";
const char  UTOPIC[] = "utopic";

const char  PLATFORM_DESKTOP[] = "Desktop";
const char  PLATFORM_DESKTOP_DISPLAYNAME[] = "Ubuntu %0";

const char  TASK_DEVICE_SCRIPT[] = "Ubuntu.Task.DeviceScript";

const char  UBUNTU_SETTINGS_ICON[] = ":/ubuntu/images/ubuntu-32.png";

//review tools
const char CLICK_REVIEWERSTOOLS_BINARY[]   = "/usr/bin/click-review";
const char CLICK_REVIEWERSTOOLS_ARGS[]     = "--sdk \"%0\"";
const char CLICK_REVIEWERSTOOLS_LOCATION[] = "/usr/bin/click-review --sdk \"%0\"";

//build configuration
const char UBUNTU_CLICK_OPEN_TERMINAL_ERROR[] = "Error when starting terminal";
const char UBUNTU_CLICK_TARGETS_REGEX[] = "^%1-(.*)-([A-Za-z0-9]+)$";
const char UBUNTU_CLICK_TARGETS_FRAMEWORK_REGEX[] = "^%1-(%2)-([A-Za-z0-9]+)$";
const char UBUNTU_CLICK_BASE_FRAMEWORK_REGEX[] = "(ubuntu-(.*)-[\\d]{1,2}\\.[\\d]{1,2}(\\.[\\d]+)?)";
const char UBUNTU_CLICK_VERSION_REGEX[] = "^DISTRIB_RELEASE=([0-9]+)\\.([0-9]+)$";
const char UBUNTU_CLICK_SERIES_REGEX[]  = "^DISTRIB_CODENAME=([A-Za-z]+)$";

const char UBUNTU_CLICK_CHROOT_SUFFIX_ENV_VAR[] = "CLICK_CHROOT_SUFFIX";
const char UBUNTU_CLICK_CHROOT_DEFAULT_NAME[] = "click"; 
const char UBUNTU_CLICK_BINARY[]  = "/usr/bin/click";
const char UBUNTU_SUDO_BINARY[]   = "/usr/bin/pkexec";

const char UBUNTU_TARGET_TOOL[]   = "%1/usdk-target.py";
const char UBUNTU_CREATE_CLICK_TARGET_ARGS[]  = "%0/usdk-target.py create -a %1 -f %2 -n %3 -p %4";
const char UBUNTU_DESTROY_CLICK_TARGET_ARGS[] = "%0/usdk-target.py destroy %1";
const char UBUNTU_UPGRADE_CLICK_TARGET_ARGS[] = "upgrade %0";
const char UBUNTU_CLICK_OPEN_TERMINAL[]       = "%0/usdk-target.py maint %1";

const char UBUNTU_CLICK_CLICK_PACKAGE_DIR[] = "%CLICK_FRAMEWORK%-%CLICK_ARCH%/click_package";


const char UBUNTU_CLICK_DELETE_TITLE[] = "Delete click chroot";
const char UBUNTU_CLICK_DELETE_MESSAGE[] = "Are you sure you want to delete this chroot?";
const char UBUNTU_CLICK_STOP_TITLE[] = "Stop click tool";
const char UBUNTU_CLICK_STOP_MESSAGE[] = "Are you sure you want to stop click? This could break your chroot!";
const char UBUNTU_CLICK_STOP_WAIT_MESSAGE[] = "Waiting for click to stop";
const char UBUNTU_CLICK_ERROR_EXIT_MESSAGE[] = "Click exited with errors, please check the output";
const char UBUNTU_CLICK_SUCCESS_EXIT_MESSAGE[] = "Click exited with no errors";
const char UBUNTU_CLICK_NOTARGETS_TITLE[] = "No click build targets available";
const char UBUNTU_CLICK_NOTARGETS_MESSAGE[] = "There are no click build targets available.\nPlease create a target in the Ubuntu option page.";
const char UBUNTU_CLICK_NOTARGETS_FRAMEWORK_MESSAGE[] = "There are no click build targets for framework %1 available.\nPlease create a target in the Ubuntu option page.";
const char UBUNTU_CLICK_SELECT_TARGET_TITLE[] = "Select build target";
const char UBUNTU_CLICK_SELECT_TARGET_LABEL[] = "Build target";
extern const char* UBUNTU_CLICK_SUPPORTED_ARCHS[];
extern const char* UBUNTU_CLICK_SUPPORTED_TARGETS[][3];

//Buildsupport
const char UBUNTU_CLICK_TOOLCHAIN_ID[]   = "UbuntuProjectManager.UbuntuGccToolChain";
const char UBUNTU_CLICK_TARGET_WRAPPER[] = "%0/qtc_target_wrapper.py";
const char UBUNTU_CLICK_HTML_BC_ID[]     = "UbuntuProjectManager.UbuntuHTML5.BuildConfiguration";
const char UBUNTU_CLICK_QML_BC_ID[]      = "UbuntuProjectManager.UbuntuQml.BuildConfiguration";
const char UBUNTU_CLICK_QML_UPDATE_TRANSL_MAKESTEP[]      = "UbuntuProjectManager.UbuntuQml.UpdateTranslationTemplateMakeStep";
const char UBUNTU_CLICK_QML_BUILD_TRANSL_MAKESTEP[]      = "UbuntuProjectManager.UbuntuQml.BuildTranslationMakeStep";
const char UBUNTU_CLICK_QML_BUILD_TRANSL_DIR[]      = "mo";

//Devicesupport
const char UBUNTU_DEVICE_TYPE_ID[] = "UbuntuProjectManager.DeviceTypeId";
const char UBUNTU_DEVICE_SSHIDENTITY[] = "ubuntudevice_id_rsa";

//Container devices
const int  UBUNTU_DESKTOP_PORT_START = 40000;
const int  UBUNTU_DESKTOP_PORT_END = 41000;
const char UBUNTU_CONTAINER_DEVICE_TYPE_ID[] = "UbuntuProjectManager.LocalDeviceTypeId.";
const char UBUNTU_CONTAINER_DEPLOY_PUBKEY_SCRIPT[] = "%0/container_publickey_deploy";
const char UBUNTU_TASK_CATEGORY_DEVICE [] = "Task.Category.Ubuntu.ContainerDevice";

//Deploysupport
const char UBUNTU_DEPLOYCONFIGURATION_ID[]       = "UbuntuProjectManager.DeployConfiguration";
const char UBUNTU_LOCAL_DEPLOYCONFIGURATION_ID[] = "UbuntuProjectManager.LocalDeployConfiguration";
const char UBUNTU_DEPLOY_UPLOADSTEP_ID[] = "UbuntuProjectManager.UploadStep";
const char UBUNTU_DEPLOY_MAKESTEP_ID[]   = "UbuntuProjectManager.UbuntuCMake.DeployMakeStep";
const char UBUNTU_CLICK_PACKAGESTEP_ID[] = "UbuntuProjectManager.ClickPackageStep";
const char UBUNTU_DEPLOY_DESTDIR[] = ".ubuntu-sdk-deploy";
const char UBUNTU_CLICK_SUCCESS_PACKAGE_REGEX[] = "^.*'(.*)'.$";

//Frameworks
const char UBUNTU_DEFAULT_QML_FRAMEWORK[]    = "ubuntu-sdk-14.10-qml";
const char UBUNTU_DEFAULT_HTML_FRAMEWORK[]   = "ubuntu-sdk-14.10-html";
const char UBUNTU_UNKNOWN_FRAMEWORK_NAME[]   = "Unknown framework";
const int  UBUNTU_UNKNOWN_FRAMEWORK_DATA     = 0xdeadbeef;

/*
 * GOLANG constants, copied to remove the need to
 * depend on the golang plugin. Remove as soon
 * as there is a released version of it
 */
const char GO_PROJECT_MIMETYPE[] = "application/x-goproject";
const char GO_PROJECT_ID[]       = "GoProjectManager.GoProject";
const char GO_PROJECT_PROJECTCONTEXT[] = "GoProject.ProjectContext";
const char GO_PROJECT_SUFFIX[] = ".goproject";
const char LANG_GO[]           = "GOLANG";
const char GO_TOOLCHAIN_ID[]   = "GoLang.Toolchain";
const char TOOLCHAIN_SETTINGS_PAGE_ID[] = "GoLang.SettingsPage";
const char GO_BUILDCONFIGURATION_ID[] = "GoLang.Buildconfiguration";
const char GO_GOSTEP_ID[] = "GoLang.BuildConfiguration.GoStep";
const char GO_RUNCONFIG_ID[] = "GoLang.GoRunConfiguration";
const char GO_SUPPORT_FEATURE[] = "GoLang.GoSupport";

const char UBUNTU_GO_BUILD_TARGETS[] = "%GOBUILDTARGETS%";

//Qtversion support
const char UBUNTU_QTVERSION_TYPE[]   = "UbuntuProjectManager.QtVersion";
const char UBUNTU_PLATFORM_NAME[]    = "Ubuntu Phone";
const char UBUNTU_PLATFORM_NAME_TR[] = QT_TRANSLATE_NOOP("UbuntuProjectManager", "Ubuntu Phone");

//Manifest Editor
const char UBUNTU_MANIFEST_MIME_TYPE[] = "application/vnd.canonical.click.manifest";
const char UBUNTU_APPARMOR_MIME_TYPE[] = "application/vnd.canonical.click.apparmor";
const char UBUNTU_MANIFEST_EDITOR_ID[] = "UbuntuProjectManager.UbuntuManifestEditor.Id";
const char UBUNTU_MANIFEST_EDITOR_CONTEXT[] = "UbuntuProjectManager.UbuntuManifestEditor.Context.Id";
const char UBUNTU_APPARMOR_EDITOR_ID[]      = "UbuntuProjectManager.UbuntuApparmorEditor.Id";
const char UBUNTU_APPARMOR_EDITOR_CONTEXT[] = "UbuntuProjectManager.UbuntuApparmorEditor.Context.Id";

//Actions
const char UBUNTU_MIGRATE_QMAKE_PROJECT[] = "UbuntuProjectManager.MigrateQMakeProject";

//TargetUpgradeManager
const char CHROOT_UPDATE_LIST_SCRIPT[] = "%1/ubuntu/scripts/qtc_chroot_get_upgrades.py %2 %3";


} // namespace Ubuntu
} // namespace Constants

#endif // UBUNTUCONSTANTS_H

