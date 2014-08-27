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
import dbus
import dbus.service
import dbus.mainloop.glib

# Runner to handle scopes
class ScopeRunner(dbus.service.Object):
    def __init__(self,appid,loop):
        self.appId = appid
        self.loop  = loop
        busName = dbus.service.BusName('com.ubuntu.SDKAppLaunch', bus = dbus.SessionBus())
        dbus.service.Object.__init__(self, busName, '/ScopeRegistryCallback')

    @dbus.service.method("com.ubuntu.SDKAppLaunch",
                         in_signature='si', out_signature='')
    def ScopeLoaded(self, name, pid):
        if(name == self.appid):
            print("Scope started: "+str(pid),flush=True)

    @dbus.service.method("com.ubuntu.SDKAppLaunch",
                         in_signature='s', out_signature='')
    def ScopeStopped(self, name):
        print("Scope stopped, exiting",flush=True)
        mainloop.quit()

    def launch(self):
        try:
            self.loop.run()
        except KeyboardInterrupt:
            pass
        return 0

    def stop(self):
        return None

# Runner to handle apps
class AppRunner:
    def __init__(self,appid,loop):
        self.appid = appid
        self.exitCode = 0
        self.message = ""
        self.loop = loop

    def on_failed(self,launched_app_id, failure_type):
        print("Received a failed event",flush=True)
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
            print("Application started: "+str(UAL.get_primary_pid(self.appid)),flush=True)

    def on_stopped(self,stopped_app_id):
        if stopped_app_id == self.appid:
            print("Stopping Application",flush=True)
            self.exitCode = 0
            self.loop.quit()

    def on_resume(self,resumed_app_id):
        if resumed_app_id == self.appid:
            print("Application was resumed",flush=True)

    def on_focus(self,focused_app_id):
        if focused_app_id == self.appid:
            print("Application was focused",flush=True)

    def launch(self):
        print ("Registering hooks",flush=True)
        UAL.observer_add_app_failed(self.on_failed)
        UAL.observer_add_app_started(self.on_started)
        UAL.observer_add_app_focus(self.on_focus)
        UAL.observer_add_app_stop(self.on_stopped)
        UAL.observer_add_app_resume(self.on_resume)

        print ("Start Application",flush=True)

        #start up the application
        UAL.start_application(self.appid)

        try:
            self.loop.run()
        except KeyboardInterrupt:
            pass

        print ("The Application exited, cleaning up")

        UAL.observer_delete_app_failed(self.on_failed)
        UAL.observer_delete_app_started(self.on_started)
        UAL.observer_delete_app_focus(self.on_focus)
        UAL.observer_delete_app_stop(self.on_stopped)
        UAL.observer_delete_app_resume(self.on_resume)

        return self.exitCode

    def stop(self):
        UAL.stop_application(self.appid)

#object = ScopeRegistryCallback(appid)
#loop.run()

def on_sigterm(runner):
    print("Received exit signal, stopping application",flush=True)
    runner.stop()

def create_procpipe(path,callback):
    if(os.path.exists(path)):
        os.unlink(path)

    os.mkfifo(path)
    pipe = os.open(path,os.O_RDONLY | os.O_NONBLOCK)

    # make pipe non-blocking:
    fl = fcntl.fcntl(pipe, fcntl.F_GETFL)
    fcntl.fcntl(pipe, fcntl.F_SETFL, fl | os.O_NONBLOCK)

    if GObject.pygobject_version < (3,7,2):
        GObject.io_add_watch(pipe,GObject.IO_IN | GObject.IO_HUP,callback)
    else:
        GLib.io_add_watch(pipe,GLib.PRIORITY_DEFAULT,GObject.IO_IN | GObject.IO_HUP,callback)

    return pipe

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

#make glib the default dbus event loop
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

# register options to the argument parser
parser = argparse.ArgumentParser(description="SDK application launcher")
parser.add_argument('clickPck',action="store")
parser.add_argument('--env', action='append', dest='environmentList', metavar="key:value", help="Adds one environment variable to the applications env" )
parser.add_argument('--cppdebug', action='store', dest='gdbPort', help="Runs the app in gdbserver listening on specified port")
parser.add_argument('--qmldebug', action='store', dest='qmlDebug', help="Value passed to the --qmldebug switch")
parser.add_argument('--hook', action='store', dest='targetHook', help="Specify the application hook to run from the click package")

options = parser.parse_args()

print("Executing: "+options.clickPck,flush=True)
print("Launcher PID: "+str(os.getpid()), file=sys.stderr,flush=True)

needs_debug_conf=False
conf_obj={}

if options.environmentList is not None:
    print("Setting env "+", ".join(options.environmentList),flush=True)
    needs_debug_conf=True
    conf_obj['env'] = {}
    for env in options.environmentList:
        envset = env.split(":")
        if(len(envset) != 2):
            continue
        conf_obj['env'][envset[0]] = envset[1]

if options.gdbPort is not None:
    needs_debug_conf=True
    conf_obj['gdbPort'] = options.gdbPort
    print("GDB Port"+options.gdbPort,flush=True)

if options.qmlDebug is not None:
    needs_debug_conf=True
    conf_obj['qmlDebug'] = options.qmlDebug
    print("QML Debug Settings:"+options.qmlDebug,flush=True)

hook_name = None
package_name = None
package_version = None
package_arch = None
manifest = None
app_id = None

#get the manifest information from the click package
try:
    manifest_json = subprocess.check_output(["click","info",options.clickPck])
    manifest=json.loads(manifest_json.decode())
except subprocess.CalledProcessError:
    print("Could not call click",file=sys.stderr,flush=True)
    sys.exit(1)

#get the hook name we want to execute
if len(manifest['hooks']) == 1:
    hook_name = list(manifest['hooks'].keys())[0]
else:
    if options.targetHook is None:
        print("There are multiple hooks in the manifest file, please specify one",flush=True,file=sys.stderr)
        sys.exit(1)
    else:
        if options.targetHook in manifest['hooks']:
            hook_name = options.targetHook
        else:
            print("Unknown hook selected",file=sys.stderr,flush=True)
            sys.exit(1)

if 'version' not in manifest:
    print("Version key is missing from the manifest file",flush=True,file=sys.stderr)
    sys.exit(1)

if 'name' not in manifest:
    print("Package name not in the manifest file",flush=True,file=sys.stderr)
    sys.exit(1)

package_name = manifest['name']
package_version = manifest['version']

#get the package arch
#<cjwatson> (well, even more strictly it would match what "dpkg-deb -f foo.click Architecture" says)
try:
    package_arch = subprocess.check_output(["dpkg","-f",options.clickPck,"Architecture"])
    package_arch = package_arch.decode()
except subprocess.CalledProcessError:
    print("Could not query architecture from the package",flush=True,file=sys.stderr)
    sys.exit(1)

#build the appid
app_id = package_name+"_"+hook_name+"_"+package_version
debug_file_name = "/tmp/"+app_id+"_debug.json"

print("AppId: "+app_id,flush=True)
print("Architecture: "+package_arch,flush=True)

#create the debug description file if required
if needs_debug_conf:
    try:
        f = open(debug_file_name, 'w')
        json.dump(conf_obj,f)
        f.close()
    except OSError:
        print("Could not create the debug description file")
        sys.exit(1)

#we have all informations, now install the click package
#@TODO check if its already installed

success = subprocess.call(["pkcon","install-local",options.clickPck,"-p"])
if success != 0:
    print("Installing the application failed",flush=True)
    sys.exit(1)

print("Application installed, executing",flush=True)

#create 2 named pipes and listen for data
stdoutPipeName = "/tmp/"+app_id+".stdout"
procStdOut = create_procpipe(stdoutPipeName,on_proc_stdout)

stderrPipeName = "/tmp/"+app_id+".stderr"
procStdErr = create_procpipe(stderrPipeName,on_proc_stderr)

loop = GLib.MainLoop()
runner = None

if "scope" in manifest['hooks'][hook_name]:
    runner = ScopeRunner(app_id,loop)
else if "desktop" in manifest['hooks'][hook_name]:
    runner = AppRunner(app_id,loop)
else:
    print("Hook is not supported, only scope and app hooks can be executed",flush=True)
    sys.exit(1)

if "unix_signal_add" in dir(GLib):
    GLib.unix_signal_add(GLib.PRIORITY_HIGH, signal.SIGTERM, on_sigterm, runner)
    GLib.unix_signal_add(GLib.PRIORITY_HIGH, signal.SIGINT, on_sigterm, runner)
    GLib.unix_signal_add(GLib.PRIORITY_HIGH, signal.SIGHUP, on_sigterm, runner)
else:
    GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGTERM, on_sigterm, runner)
    GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGINT, on_sigterm, runner)
    GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGHUP, on_sigterm, runner)

#execute the hook, this will not return before the app or scope finished to run
exitCode = runner.launch()

success = subprocess.call(["pkcon","remove",package_name+";"+package_version+";"+package_arch+";local:click","-p"])
if success != 0:
    print("Uninstalling the application failed",flush=True)

if needs_debug_conf:
    try:
        if os.path.isfile(debug_file_name):
            os.remove(debug_file_name)
    except:
        print("Could not remove the debug description file: "+debug_file_name+"\n Please delete it manually",flush=True,file=sys.stderr)

os.close(procStdOut)
os.unlink(stdoutPipeName)
os.close(procStdErr)
os.unlink(stderrPipeName)

sys.exit(exitCode)
