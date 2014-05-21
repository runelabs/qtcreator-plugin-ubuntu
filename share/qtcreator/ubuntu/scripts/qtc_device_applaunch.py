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

from gi.repository import GLib, UpstartAppLaunch
import json
import os
import sys
import signal
import subprocess
import argparse

def on_sigterm(state):
    print("Received exit signal, stopping application")
    UpstartAppLaunch.stop_application(state['expected_app_id'])

def on_failed(launched_app_id, failure_type, state):
    print("Received a failed message")
    if launched_app_id == state['expected_app_id']:
        if failure_type == UpstartAppLaunch.AppFailed.CRASH:
            state['message']  = 'Application crashed.'
            state['exitCode'] = 1
        elif failure_type == UpstartAppLaunch.AppFailed.START_FAILURE:
            state['message'] = 'Application failed to start.'
            state['exitCode'] = 1

        state['loop'].quit()

def on_started(launched_app_id, state):
    if launched_app_id == state['expected_app_id']:
        print("Application started")

def on_stopped(stopped_app_id, state):
    if stopped_app_id == state['expected_app_id']:
        print("Stopping Application")
        state['exitCode'] = 0
        state['loop'].quit()


# register options to the argument parser
parser = argparse.ArgumentParser(description="SDK application launcher")
parser.add_argument('clickPck',action="store")
parser.add_argument('--env', action='append', dest='environmentList', metavar="key:value", help="Adds one environment variable to the applications env" )
parser.add_argument('--cppdebug', action='store', dest='gdbPort', help="Runs the app in gdbserver listening on specified port")
parser.add_argument('--qmldebug', action='store', dest='qmlPort', help="Runs the app in qml debugging mode listening on specified port")
parser.add_argument('--hook', action='store', dest='targetHook', help="Specify the application hook to run from the click package")

options = parser.parse_args()

print("Executing: "+options.clickPck)

if options.environmentList is not None:
    print("Setting env "+", ".join(options.environmentList))

if options.gdbPort is not None:
    print("GDB Port"+options.gdbPort)

if options.qmlPort is not None:
    print("QML Port"+options.qmlPort)


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
    print("Could not call click")
    sys.exit(1)

#get the hook name we want to execute
if len(manifest['hooks']) == 1:
    hook_name = list(manifest['hooks'].keys())[0]
else:
    if options.targetHook is None:
        print("There are multiple hooks in the manifest file, please specify one")
        sys.exit(1)
    else:
        if options.targetHook in manifest['hooks']:
            hook_name = options.targetHook
        else:
            print("Unknown hook selected")
            sys.exit(1)

if 'version' not in manifest:
    print("Version key is missing from the manifest file")
    sys.exit(1)

if 'name' not in manifest:
    print("Package name not in the manifest file")
    sys.exit(1)

package_name = manifest['name']
package_version = manifest['version']

#get the package arch
#<cjwatson> (well, even more strictly it would match what "dpkg-deb -f foo.click Architecture" says)
try:
    package_arch = subprocess.check_output(["dpkg","-f",options.clickPck,"Architecture"])
    package_arch = package_arch.decode()
except subprocess.CalledProcessError:
    print("Could not query architecture from the package")
    sys.exit(1)

#build the appid
app_id = package_name+"_"+hook_name+"_"+package_version

print("AppId: "+app_id)
print("Architecture: "+package_arch)

#we have all informations, now install the click package
#@TODO check if its already installed

success = subprocess.call(["pkcon","install-local",options.clickPck])
if success != 0:
    print("Installing the application failed")
    sys.exit(1)

print("Application installed, executing")

state = {}
state['loop'] = GLib.MainLoop()
state['message'] = ''
state['expected_app_id'] = app_id
state['exitCode'] = 0

print ("Registering hooks")
GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGTERM, on_sigterm, state)
GLib.unix_signal_add_full(GLib.PRIORITY_HIGH, signal.SIGINT, on_sigterm, state)

UpstartAppLaunch.observer_add_app_failed(on_failed, state)
UpstartAppLaunch.observer_add_app_started(on_started, state)
#UpstartAppLaunch.observer_add_app_focus(on_started, state)
UpstartAppLaunch.observer_add_app_stop(on_stopped, state)

print ("Start Application")

#start up the application
UpstartAppLaunch.start_application(app_id)
state['loop'].run()

print ("The Application exited, cleaning up")

UpstartAppLaunch.observer_delete_app_failed(on_failed)
UpstartAppLaunch.observer_delete_app_started(on_started)
#UpstartAppLaunch.observer_delete_app_focus(on_started)
UpstartAppLaunch.observer_delete_app_stop(on_stopped)

success = subprocess.call(["pkcon","remove",package_name+";"+package_version+";"+package_arch+";local:click"])
if success != 0:
    print("Uninstalling the application failed")
    sys.exit(1)

sys.exit(state['exitCode'])
