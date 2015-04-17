#!/usr/bin/env python3
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# QTC device chroot wrapper
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

# This script basically starts the tool passed in sys.argv[0] in a chroot
# and tries to map all path that are printed to the console to the host
# os. It is primarily used to teach qtcreator how to handle the chroots
# approach we use in Ubuntu. The tool needs to be in PATH when logged into
# the chroot. All arguments are forwarded

import sys
import os
import shutil
import subprocess
import select
import re
import fcntl
import uuid
import signal
import dbus

#find out the directory holding the link
dirname = os.path.basename(os.path.dirname(os.path.abspath(sys.argv[0])))
idx = dirname.rfind("-")
if(idx is -1):
    print("The parent directory must contain the click chroot name (e.g ubuntu-sdk-14.10-armhf)")
    sys.exit(1)

#find out the path this script is in, required for local plugin installation
scriptpath = os.path.dirname(os.path.realpath(__file__))

#get the click prefix from env, or use click if the var is not set
#if the variable is set we do not try to reach the click-chroot-agent
chroot_name_prefix = os.getenv('CLICK_CHROOT_SUFFIX', "click")

architecture = dirname[idx+1:]
framework    = dirname[0:idx]

args    = sys.argv[1:]
command = os.path.basename(sys.argv[0])

if (command.startswith("qt5-qmake")):
    command_path="/var/lib/schroot/chroots/"+chroot_name_prefix+"-"+dirname+"/usr/bin/"+command
    if (not os.path.isfile(command_path)):
        legacy_script = scriptpath+"/qtc_chroot_qmake_legacy"
        success = subprocess.call(["/bin/bash",legacy_script,framework,architecture,chroot_name_prefix]+args,stdout=sys.stdout,stderr=sys.stderr)
        sys.exit(success)

subproc    = None
session_id = ""
click      = shutil.which("click")
pre_spawned_session = False

if( click is None ):
    print("Could not find click in the path, please make sure it is installed")
    sys.exit(1)

def _doMapAllPaths (matchobj):
    return matchobj.group(1)+"/var/lib/schroot/chroots/"+chroot_name_prefix+"-"+dirname+"/"+matchobj.group(2)

def mapPaths (text):
    paths = ["var","bin","boot","dev","etc","lib","lib64","media","mnt","opt","proc","root","run","sbin","srv","sys","usr"]
    for path in paths:
        text = re.sub("(^|[^\w+]|\s+|-\w)\/("+path+")", _doMapAllPaths , text)
    return text

def exit_gracefully(arg1,arg2):
    if(subproc is not None):
        try:
            subproc.kill()
        except ProcessLookupError:
            #process is already gone
            pass

    if (not pre_spawned_session):
        subprocess.call([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"end-session",session_id],stdout=subprocess.DEVNULL)

    sys.exit(1)

signal.signal(signal.SIGTERM, exit_gracefully)
signal.signal(signal.SIGINT , exit_gracefully)
signal.signal(signal.SIGHUP , exit_gracefully)

#only ask the chroot-agent if we use the default click prefix
if chroot_name_prefix == "click":
    try:
        sessionBus = dbus.SessionBus()
        clickChrootAgent = sessionBus.get_object('com.ubuntu.sdk.ClickChrootAgent','/com/ubuntu/sdk/ClickChrootAgent')
        clickChrootAgentIFace = dbus.Interface(clickChrootAgent,dbus_interface='com.ubuntu.sdk.ClickChrootAgent')
        session_id = clickChrootAgentIFace.spawnSession(framework,architecture)
    except dbus.exceptions.DBusException:
        session_id = ""

if (len(session_id) == 0):
    session_id   = str(uuid.uuid4())
    pre_spawned_session = False
else:
    pre_spawned_session = True

if ( not pre_spawned_session ):
    success = subprocess.call([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"begin-session",session_id],stdout=subprocess.DEVNULL)

subproc = subprocess.Popen([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"run","-n",session_id]+[command]+args,stdout=subprocess.PIPE, stderr=subprocess.PIPE)

stdout = ""
stderr = ""

#even though after select a read should never block, it can happen sometimes
#so better make them nonblocking
stdoutfd = subproc.stdout.fileno()
fl = fcntl.fcntl(stdoutfd, fcntl.F_GETFL)
fcntl.fcntl(stdoutfd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

stderrfd = subproc.stderr.fileno()
fl = fcntl.fcntl(stderrfd, fcntl.F_GETFL)
fcntl.fcntl(stderrfd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

while True:
    reads = [stdoutfd, stderrfd]
    ret = select.select(reads, [], [])

    for fd in ret[0]:
        if fd == subproc.stdout.fileno():
            read = subproc.stdout.read()
            stdout += read.decode()
            try:
                while(True):
                    idx = stdout.index("\n")
                    line = stdout[0:idx+1]
                    stdout = stdout[idx+1:]
                    sys.stdout.write(mapPaths(line))
            except ValueError:
                pass
        if fd == subproc.stderr.fileno():
            read = subproc.stderr.read()
            stderr += read.decode()
            try:
                while(True):
                    idx = stderr.index("\n")
                    line = stderr[0:idx+1]
                    stderr = stderr[idx+1:]
                    sys.stderr.write(mapPaths(line))
            except ValueError:
                pass

    if subproc.poll() != None:
        break

if(len(stdout) != 0):
    sys.stdout.write(mapPaths(stdout))

if(len(stderr) != 0):
    sys.stderr.write(mapPaths(stderr))

if (not pre_spawned_session):
    subprocess.call([click, "chroot","-a",architecture,"-f",framework,"-n",chroot_name_prefix,"end-session",session_id],stdout=subprocess.DEVNULL)

sys.exit(subproc.returncode)
