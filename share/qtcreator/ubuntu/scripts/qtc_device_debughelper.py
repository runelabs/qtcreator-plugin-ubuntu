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
import sys
import subprocess
import shlex
import shutil

app_id = os.environ.get('APP_ID')
args   = shlex.split(sys.argv[1])
env    = os.environ

effective_cmd = command = shutil.which(args.pop(0))
if command is None:
    print("Executable was not found in the PATH")
    sys.exit(1)

if app_id is None:
    sys.exit(1)

debug_file_name = "/tmp/"+app_id+"_debug.json"

if os.path.isfile(debug_file_name):
    f = open(debug_file_name,"r")
    try:
        debug_settings = json.load(f)
    except:
        print("Could not load the settings file")
        sys.exit(1)

    if "qmlDebug" in debug_settings:
        args.insert(0,"-qmljsdebugger="+debug_settings["qmlDebug"])

    if "gdbPort" in debug_settings:
        effective_cmd = shutil.which("gdbserver")
        if effective_cmd is None:
            print("gdbserver was not found in the PATH")
            sys.exit(1)
        args.insert(0,":"+debug_settings["gdbPort"])
        args.insert(1,command)

    if "env" in debug_settings:
        for key in debug_settings["env"]:
            env[key] = debug_settings["env"][key]

#execv wants the command again in the arguments
args.insert(0,effective_cmd)

print("Executing: ",effective_cmd,args)

#flush all descriptors
sys.stdout.flush()

#replace us with the new process
os.execv(effective_cmd,args)
