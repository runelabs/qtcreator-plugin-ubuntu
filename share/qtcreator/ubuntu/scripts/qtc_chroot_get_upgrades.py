#!/usr/bin/env python3
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# QTC chroot update check
# Copyright (C) 2015 Canonical
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

import signal
import subprocess
import sys
import os
import uuid
import shutil

def splitIgnoreEmptyParts(s, delim=None):
    return [x for x in s.split(delim) if x]

if (len(sys.argv) < 3):
    print("Useage: qtc_chroot_get_upgrades <architecture> <framework>")
    sys.exit(-1)

click      = shutil.which("click")
session_id = ""
chroot_name_prefix = os.getenv('CLICK_CHROOT_SUFFIX', "click")

architecture = sys.argv[1]
framework    = sys.argv[2]
subproc      = None

if (len(session_id) == 0):
    session_id   = str(uuid.uuid4())
    pre_spawned_session = False
else:
    pre_spawned_session = True

f=open("/tmp/debug_"+session_id,"w")

def endSession():
    subprocess.call([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"end-session",session_id],stdout=f,stderr=f)
    f.close()

def exit_gracefully(arg1,arg2):
    if(subproc is not None):
        subproc.kill()
    endSession()
    sys.exit(-1)

signal.signal(signal.SIGTERM, exit_gracefully)
signal.signal(signal.SIGINT , exit_gracefully)
signal.signal(signal.SIGHUP , exit_gracefully)

if ( not pre_spawned_session ):
    success = subprocess.call([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"begin-session",session_id],stdout=f,stderr=f)

subproc = subprocess.Popen([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"maint","-n",session_id
                           ,"env","LC_ALL=C","apt-get","update"],stdout=f,stderr=f)
subproc.wait()
subproc = subprocess.Popen([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"maint","-n",session_id
                           ,"env","LC_ALL=C","apt","list","--upgradable"],stdout=subprocess.PIPE,stderr=f, universal_newlines=True)
stdout, stderr = subproc.communicate()
endSession()

packages = splitIgnoreEmptyParts(stdout,"\n")
if(len(packages) == 0):
    sys.exit(0)

packages.pop(0)
sys.exit(len(packages))





