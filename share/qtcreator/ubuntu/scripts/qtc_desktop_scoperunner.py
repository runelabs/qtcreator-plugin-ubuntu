#!/usr/bin/python3
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

import json
import os
import os.path
import subprocess
import argparse
import shutil
import sys
import re

# register options to the argument parser
parser = argparse.ArgumentParser(description="SDK Scope launcher")
parser.add_argument('scope_ini',action="store")
parser.add_argument('--cppdebug', action='store', dest='gdbPort', help="Runs the app in gdbserver listening on specified port")

options = parser.parse_args()

if not os.path.isfile(options.scope_ini):
    print("Scope ini file does not exist")
    sys.exit(1)

# remove the .ini
app_id = os.path.basename(options.scope_ini)[:-4]

regex = re.compile("_");
try:
    (packagename,hookname) = regex.split(app_id)
except ValueError as err:
    print("Invalid Application ID "+app_id+" "+repr(err),file=sys.stderr)
    sys.exit(1)

#the scope tmpdir, as this is desktop its always unconfined (for now)
tmpdir  = os.path.expanduser('~')+"/.local/share/unity-scopes/unconfined/"+packagename+"/"
if(not os.path.exists(tmpdir)):
    os.makedirs(tmpdir)

print ("ScopeRunner> Setting up environment")
print ("ScopeRunner> TmpDir:      "+tmpdir)
print ("ScopeRunner> AppId:       "+app_id)

command = shutil.which("unity-scope-tool")

if command is None:
    print("ScopeRunner> unity-scope-tool was not found in the PATH. Please run \"apt-get install unity-scope-tool\"")
    sys.exit(1)

if app_id is None:
    sys.exit(1)

#the config file
debug_file_name = tmpdir+app_id+"_debug.json"

needs_debug_conf=False
conf_obj={}

if options.gdbPort is not None:
    needs_debug_conf=True

    print("Sdk-Launcher> Checking if gdbserver is installed...")
    gdbserver_path = shutil.which("gdbserver")
    if gdbserver_path is None:
        print("Sdk-Launcher> gdbserver was not found in the PATH.")
        print("Sdk-Launcher> Please install the gdbserver package on the phone.")
        sys.exit(1)

    conf_obj['gdbPort'] = options.gdbPort
    print("Sdk-Launcher> GDB Port"+options.gdbPort,flush=True)

#create the debug description file if required or delete it if not
if needs_debug_conf:
    try:
        f = open(debug_file_name, 'w')
        json.dump(conf_obj,f)
        f.close()
    except OSError:
        #error we need to stop
        print("Sdk-Launcher> Could not create the debug description file")
        sys.exit(1)
elif os.path.isfile(debug_file_name):
    os.remove(debug_file_name)

print ("Debug-helper> Environment initialized, starting the application")
print ("Debug-helper> Executing "+command+" "+options.scope_ini)

#flush all descriptors
sys.stdout.flush()
sys.stderr.flush()

#replace us with the new process
os.execv(command,[command,options.scope_ini])
