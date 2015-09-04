#!/usr/bin/env python3
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# QTC device applauncher
# Copyright (C) 2014 Canonical
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Author: Benjamin Zeller <benjamin.zeller@canonical.com>

import gi
from gi.repository import GLib
from gi.repository import GObject
from gi.repository import Click
from click.json_helpers import json_array_to_python

try:
    gi.Repository.get_default().require("UbuntuAppLaunch")
    from gi.repository import UbuntuAppLaunch as UAL
except:
    #fall back to the old name
    from gi.repository import UpstartAppLaunch as UAL

import json
import os
import sys
import signal
import subprocess
import argparse
import fcntl
import shutil
import dbus
import dbus.service
import dbus.mainloop.glib

#make glib the default dbus event loop
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

#--------------------------- globals ------------------------------

#buffer for the syslog output, always contains the last or parts of the last line 
syslogBuffer=""
hook_name = None
package_name = None
package_version = None
package_arch = None
manifest = None
app_id = None
tmp_dir = "/tmp/"
apparmor_path = None
skip_apparmor_denials = 3

# Runner to handle scopes
class ScopeRunner(dbus.service.Object):
    def __init__(self,appid,loop):
        busName = dbus.service.BusName('com.ubuntu.SDKAppLaunch', bus = dbus.SessionBus())
        dbus.service.Object.__init__(self, busName, '/ScopeRegistryCallback')
        self.appId = appid
        self.loop  = loop

    @dbus.service.method("com.ubuntu.SDKAppLaunch",in_signature='si', out_signature='')
    def ScopeLoaded(self, name, pid):
        if(name == self.appId):
            #Do NOT change this line, its interpreted by the IDE
            print("Sdk-Launcher> Application started: "+str(pid), file=sys.stderr,flush=True)

    @dbus.service.method("com.ubuntu.SDKAppLaunch",in_signature='s', out_signature='')
    def ScopeStopped(self, name):
        print("Sdk-Launcher> Scope stopped, exiting",flush=True)
        self.loop.quit()

    def launch(self):
        try:
            scopeUrl = "scope://"+self.appId
            if(self._dispatchUrl(scopeUrl)):
                self.loop.run()
                #just make the default scope visible again
                self._dispatchUrl("scope://clickscope")
            else:
                print("Sdk-Launcher> Error: Could start the scope.",flush=True,file=sys.stderr)
                return 1
        except KeyboardInterrupt:
            pass
        return 0

    def _dispatchUrl(self,url):
        try:
            bus = dbus.SessionBus()
            urlDispatcher = bus.get_object('com.canonical.URLDispatcher',
                                           '/com/canonical/URLDispatcher')

            urlDispatcher.DispatchURL(url,"",
                                      dbus_interface='com.canonical.URLDispatcher')
        except dbus.DBusException:
            print("Sdk-Launcher> Error: Could not start the scope.",flush=True,file=sys.stderr)
            return False
        return True

    def stop(self):
        self.loop.quit()
        return None

# Runner to handle apps
class AppRunner:
    def __init__(self,appid,loop):
        self.appid = appid
        self.exitCode = 0
        self.message = ""
        self.loop = loop

    def on_failed(self,launched_app_id, failure_type):
        print("Sdk-Launcher> Received a failed event",flush=True)
        if launched_app_id == self.appid:
            if failure_type == UAL.AppFailed.CRASH:
                self.message  = 'Application crashed.'
                self.exitCode = 1
            elif failure_type == UAL.AppFailed.START_FAILURE:
                self.message  = 'Application failed to start.'
                self.exitCode = 1
            self.loop.quit()

    def on_started(self,launched_app_id):
        if launched_app_id == self.appid:
            #Do NOT change this line, its interpreted by the IDE
            print("Sdk-Launcher> Application started: "+str(UAL.get_primary_pid(self.appid)), file=sys.stderr,flush=True)

    def on_stopped(self,stopped_app_id):
        if stopped_app_id == self.appid:
            print("Sdk-Launcher> Stopping Application",flush=True)
            self.exitCode = 0
            self.loop.quit()

    def on_resume(self,resumed_app_id):
        if resumed_app_id == self.appid:
            print("Sdk-Launcher> Application was resumed",flush=True)

    def on_focus(self,focused_app_id):
        if focused_app_id == self.appid:
            print("Sdk-Launcher> Application was focused",flush=True)

    def launch(self):
        UAL.observer_add_app_failed(self.on_failed)
        UAL.observer_add_app_started(self.on_started)
        UAL.observer_add_app_focus(self.on_focus)
        UAL.observer_add_app_stop(self.on_stopped)
        UAL.observer_add_app_resume(self.on_resume)

        #start up the application
        UAL.start_application(self.appid)

        try:
            self.loop.run()
        except KeyboardInterrupt:
            pass

        print ("Sdk-Launcher> The Application exited, cleaning up")

        UAL.observer_delete_app_failed(self.on_failed)
        UAL.observer_delete_app_started(self.on_started)
        UAL.observer_delete_app_focus(self.on_focus)
        UAL.observer_delete_app_stop(self.on_stopped)
        UAL.observer_delete_app_resume(self.on_resume)

        return self.exitCode

    def stop(self):
        UAL.stop_application(self.appid)

def on_sigterm(runner):
    print("Sdk-Launcher> Received exit signal, stopping application",flush=True)
    runner.stop()

def prepareFileHandle(handle, callback):
    # make handle non-blocking:
    fl = fcntl.fcntl(handle, fcntl.F_GETFL)
    fcntl.fcntl(handle, fcntl.F_SETFL, fl | os.O_NONBLOCK)

    if GObject.pygobject_version < (3,7,2):
        GObject.io_add_watch(handle,GObject.IO_IN | GObject.IO_HUP,callback)
    else:
        GLib.io_add_watch(handle,GLib.PRIORITY_DEFAULT,GObject.IO_IN | GObject.IO_HUP,callback)

    return handle

def create_procpipe(path,callback):
    if(os.path.exists(path)):
        os.unlink(path)

    os.mkfifo(path)
    pipe = os.open(path,os.O_RDONLY | os.O_NONBLOCK)

    return prepareFileHandle(pipe, callback)

def readPipe(pipe):
    output=""
    while True:
        try:
            output += os.read(pipe,256).decode();
        except OSError as err:
            if err.errno == os.errno.EAGAIN or err.errno == os.errno.EWOULDBLOCK:
                break
            else:
                raise  # something else has happened -- better reraise

        if len(output) <= 0 :
            break

    return output;

def on_proc_stdout(file, condition):
    output = readPipe(file)
    print (output,end="",flush=True)
    return True

def on_proc_stderr(file, condition):
    output = readPipe(file)
    print (output ,file=sys.stderr,end="",flush=True)
    return True

def create_filelistener(path, callback):
    if(not os.path.exists(path)):
        return None

    handle = os.open(path,os.O_RDONLY | os.O_NONBLOCK)
    os.lseek(handle,0,os.SEEK_END)

    return prepareFileHandle(handle, callback)

def filter_syslog_line(line):
    global skip_apparmor_denials

    if app_id in line:
        if skip_apparmor_denials > 0:
            skip_apparmor_denials-=1
            return
        sys.stderr.write("Sdk-Launcher> There has been a AppArmor denial for your application.\n")
        sys.stderr.write("Sdk-Launcher> Most likely it is missing a policy in the AppArmor file.\n")
        
        # do not change this line, it is interpreted by QtCreator 
        sys.stderr.write("Syslog> "+line)

def on_syslog_update(fd, condition):
    global syslogBuffer

    while True:
        chunk = os.read(fd,256).decode();
        if (len(chunk) == 0):
            break

        syslogBuffer += chunk
        
    if len(syslogBuffer) <= 0 :
        return True

    #read the buffer and filter every complete line    
    try:
        while(True):
            idx = syslogBuffer.index("\n")
            line = syslogBuffer[0:idx+1]
            syslogBuffer = syslogBuffer[idx+1:]
            filter_syslog_line(line)
    except ValueError:
        pass
    
    return True

def is_confined (manifest_obj, hook_name):
    if "apparmor" not in manifest_obj['hooks'][hook_name]:
        raise Exception("Error: Invalid Manifest file")
    if "name" not in manifest_obj:
        raise Exception("Error: Invalid Manifest file")

    install_dir   = "/opt/click.ubuntu.com/"
    apparmor_path = install_dir+manifest_obj['name']+"/current/"+manifest_obj['hooks'][hook_name]["apparmor"]

    if (not os.path.isfile(apparmor_path)):
        raise Exception("Error: Apparmor path is not valid: "+apparmor_path)

    try:
        json_file = open(apparmor_path,"r")
        apparmor=json.load(json_file)
        json_file.close()
    except Exception as err:
        raise Exception("Error: Could not read the apparmor file "+str(err))

    if ("template" in apparmor):
        return apparmor["template"] != "unconfined"

    #without a template the app is always confined
    return True;

# register options to the argument parser
parser = argparse.ArgumentParser(description="SDK application launcher")
parser.add_argument('clickPck',action="store")
parser.add_argument('--env', action='append', dest='environmentList', metavar="key:value", help="Adds one environment variable to the applications env" )
parser.add_argument('--cppdebug', action='store', dest='gdbPort', help="Runs the app in gdbserver listening on specified port")
parser.add_argument('--qmldebug', action='store', dest='qmlDebug', help="Value passed to the --qmldebug switch")
parser.add_argument('--hook', action='store', dest='targetHook', help="Specify the application hook to run from the click package")
parser.add_argument('--force-install', action='store_true', dest='forceInstall', help="Do not check if the click package is already installed", default=False)
parser.add_argument('--no-uninstall', action='store_true', dest='noUninstall', help="Do remove the click package after execution is finished", default=False)

options = parser.parse_args()

print("Sdk-Launcher> Executing:     "+options.clickPck,flush=True)
print("Sdk-Launcher> Force Install: "+str(options.forceInstall),flush=True)
print("Sdk-Launcher> Skip Uninstall:"+str(options.noUninstall),flush=True)

#Do NOT change this line, its interpreted by the IDE
print("Sdk-Launcher> Launcher PID: "+str(os.getpid()), file=sys.stderr,flush=True)

needs_debug_conf=False
conf_obj={}

if options.environmentList is not None:
    print("Sdk-Launcher> Setting env "+", ".join(options.environmentList),flush=True)
    needs_debug_conf=True
    conf_obj['env'] = {}
    for env in options.environmentList:
        envset = env.split(":")
        if(len(envset) != 2):
            continue
        conf_obj['env'][envset[0]] = envset[1]

if options.gdbPort is not None:
    needs_debug_conf=True

    print("Sdk-Launcher> Checking if gdbserver is installed...",flush=True)
    gdbserver_path = shutil.which("gdbserver")
    if gdbserver_path is None:
        print("Sdk-Launcher> gdbserver was not found in the PATH.",file=sys.stderr,flush=True)
        print("Sdk-Launcher> Please install the gdbserver package on the phone.",file=sys.stderr,flush=True)
        sys.exit(1)

    conf_obj['gdbPort'] = options.gdbPort
    print("Sdk-Launcher> GDB Port"+options.gdbPort,flush=True)

if options.qmlDebug is not None:
    needs_debug_conf=True
    conf_obj['qmlDebug'] = options.qmlDebug
    print("Sdk-Launcher> QML Debug Settings:"+options.qmlDebug,flush=True)

#get the manifest information from the click package
try:
    manifest_json = subprocess.check_output(["click","info",options.clickPck])
    manifest=json.loads(manifest_json.decode())
except subprocess.CalledProcessError:
    print("Sdk-Launcher> Could not call click",file=sys.stderr,flush=True)
    sys.exit(1)

#get the hook name we want to execute
if len(manifest['hooks']) == 1:
    hook_name = list(manifest['hooks'].keys())[0]
else:
    if options.targetHook is None:
        print("Sdk-Launcher> There are multiple hooks in the manifest file, please specify one",flush=True,file=sys.stderr)
        sys.exit(1)
    else:
        if options.targetHook in manifest['hooks']:
            hook_name = options.targetHook
            apparmor_path = manifest['hooks'][hook_name]["apparmor"]
        else:
            print("Sdk-Launcher> Unknown hook selected",file=sys.stderr,flush=True)
            sys.exit(1)

if 'version' not in manifest:
    print("Sdk-Launcher> Version key is missing from the manifest file",flush=True,file=sys.stderr)
    sys.exit(1)

if 'name' not in manifest:
    print("Sdk-Launcher> Package name not in the manifest file",flush=True,file=sys.stderr)
    sys.exit(1)

package_name = manifest['name']
package_version = manifest['version']

#get the package arch
#<cjwatson> (well, even more strictly it would match what "dpkg-deb -f foo.click Architecture" says)
try:
    package_arch = subprocess.check_output(["dpkg","-f",options.clickPck,"Architecture"])
    package_arch = package_arch.decode()
except subprocess.CalledProcessError:
    print("Sdk-Launcher> Could not query architecture from the package",flush=True,file=sys.stderr)
    sys.exit(1)

#check if the app is already installed on the device, so we do not break existing installations
db = Click.DB()
db.read(db_dir=None)
arr = json_array_to_python(db.get_manifests(all_versions=False))
for installAppManifest in arr:
    if installAppManifest["name"] == package_name:
        if (not options.forceInstall):
            print("Sdk-Launcher> Error: This application is already installed on the device, uninstall it or temporarily change the name in the manifest.json file!",flush=True,file=sys.stderr)
            sys.exit(100)
        else:
            print("Sdk-Launcher> Uninstalling already installed package (--force-install)")
            success = subprocess.call(["pkcon","remove",package_name+";"+package_version+";"+package_arch+";local:click","-p"],stdout=subprocess.DEVNULL)
            if success != 0:
                print("Sdk-Launcher> Uninstalling the application failed",flush=True)
                # Continue even though we could not uninstall the app, the user wanted to override it anyway
                # but for scopes we need to stop, because the scope may still be running and we need a clean state
                if "scope" in manifest['hooks'][hook_name]:
                    sys.exit(1)

#build the appid
app_id   = None
debug_file_name = None
app_mode  = None
powerd    = None

loop = GLib.MainLoop()
runner = None

if "scope" in manifest['hooks'][hook_name]:
    app_mode = False
    app_id   = package_name+"_"+hook_name
    runner   = ScopeRunner(app_id,loop)
    if(not os.path.exists(tmp_dir)):
        os.mkdir(tmp_dir)

elif "desktop" in manifest['hooks'][hook_name]:
    app_mode = True
    app_id   = package_name+"_"+hook_name+"_"+package_version
    runner   = AppRunner(app_id,loop)
else:
    print("Sdk-Launcher> Hook is not supported, only scope and app hooks can be executed",flush=True)
    sys.exit(1)

print("Sdk-Launcher> Installing application .....",flush=True)
#we have all informations, now install the click package
success = subprocess.call(
    ["pkcon","--allow-untrusted","install-local",options.clickPck,"-p"])
if success != 0:
    print("Sdk-Launcher> Installing the application failed",flush=True)
    sys.exit(1)

print("Sdk-Launcher> Application installed successfully",flush=True)

debug_file_name = None
stdoutPipeName  = None
procStdOut      = None
stderrPipeName  = None
procStdErr      = None

try:
    confined = is_confined(manifest,hook_name)

    if (app_mode is True and confined):
        tmp_dir  = os.path.expanduser('~')+"/.local/share/"+package_name+"/"
    elif (app_mode is True and not confined):
        tmp_dir  = os.path.expanduser('~')+"/.local/share/"+package_name+"/"
    elif (app_mode is False and confined):
        tmp_dir  = os.path.expanduser('~')+"/.local/share/unity-scopes/leaf-net/"+package_name+"/"
    elif (app_mode is False and not confined):
        tmp_dir  = os.path.expanduser('~')+"/.local/share/unity-scopes/unconfined/"+package_name+"/"
    else:
        #error we need to stop
        raise Exception("There was a error specifying the communication directory.")

    debug_file_name = tmp_dir+app_id+"_debug.json"

    if(not os.path.exists(tmp_dir)):
        os.makedirs(tmp_dir)

    print("Sdk-Launcher> AppId:                   "+app_id,flush=True)
    print("Sdk-Launcher> Architecture:            "+package_arch,flush=True,end="")
    print("Sdk-Launcher> Application confined:    "+str(confined),flush=True)
    print("Sdk-Launcher> Communication directory: "+tmp_dir,flush=True)

    #create the debug description file if required
    if needs_debug_conf:
        try:
            f = open(debug_file_name, 'w')
            json.dump(conf_obj,f)
            f.close()
        except OSError:
            #error we need to stop
            raise Exception("Could not create the debug description file")

    #create 2 named pipes and listen for data
    stdoutPipeName = tmp_dir+app_id+".stdout"
    procStdOut = create_procpipe(stdoutPipeName,on_proc_stdout)

    stderrPipeName = tmp_dir+app_id+".stderr"
    procStdErr = create_procpipe(stderrPipeName,on_proc_stderr)

    syslogFileName = "/var/log/syslog"
    syslogHandle = create_filelistener("/var/log/syslog",on_syslog_update)

    if "unix_signal_add" in dir(GLib):
        GLib.unix_signal_add(GLib.PRIORITY_HIGH, signal.SIGTERM, on_sigterm, runner)
        GLib.unix_signal_add(GLib.PRIORITY_HIGH, signal.SIGINT, on_sigterm, runner)
        GLib.unix_signal_add(GLib.PRIORITY_HIGH, signal.SIGHUP, on_sigterm, runner)
    else:
        GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGTERM, on_sigterm, runner)
        GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGINT, on_sigterm, runner)
        GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGHUP, on_sigterm, runner)

    #unlock the phone
    powerd = subprocess.Popen(["powerd-cli", "display", "on"],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    sessionBus   = dbus.SessionBus()
    unityGreeter = sessionBus.get_object('com.canonical.UnityGreeter','/')
    unityGreeterIFace = dbus.Interface(unityGreeter,dbus_interface='com.canonical.UnityGreeter')
    unityGreeterIFace.HideGreeter()

    #execute the hook, this will not return before the app or scope finished to run
    exitCode = runner.launch()

except Exception as err:
    print(repr(err),flush=True)
    exitCode = 1

if(powerd):
    powerd.terminate()

#clean up the debug conf file if it still exists
if needs_debug_conf:
    try:
        if (debug_file_name != None and os.path.isfile(debug_file_name)):
            os.remove(debug_file_name)
    except:
        print("Sdk-Launcher> Could not remove the debug description file: "+debug_file_name+"\n Please delete it manually",flush=True,file=sys.stderr)

#close the pipes
if (stdoutPipeName != None and os.path.exists(stdoutPipeName)):
    os.close(procStdOut)
    os.unlink(stdoutPipeName)

if (stderrPipeName != None and os.path.exists(stderrPipeName)):
    os.close(procStdErr)
    os.unlink(stderrPipeName)

if (syslogHandle):
    os.close(syslogHandle)

if (options.noUninstall):
    print("Sdk-Launcher> Skipping uninstall step (--no-uninstall)")
else:
    success = subprocess.call(["pkcon","remove",package_name+";"+package_version+";"+package_arch+";local:click","-p"],stdout=subprocess.DEVNULL)
    if success != 0:
        print("Sdk-Launcher> Uninstalling the application failed",flush=True)

print("Sdk-Launcher> Finished",flush=True)
sys.exit(exitCode)
