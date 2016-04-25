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
import os.path
import shutil
import subprocess
import select
import re
import fcntl
import signal
import getpass

#find out the directory holding the link
container = os.path.basename(os.path.dirname(os.path.abspath(sys.argv[0])))

#find out the path this script is in, required for local plugin installation
scriptpath = os.path.dirname(os.path.realpath(__file__))

args    = sys.argv[1:]
command = os.path.basename(sys.argv[0])

#ugly hack to prevent from the moc error when running in the chroots
if (command == "cmake"):
    if any("help" not in s for s in args):
        work_dir = os.getcwd()
        if (os.path.exists(work_dir+"/CMakeCache.txt")):
            print("-- Removing build artifacts")
            shutil.rmtree(work_dir+'/CMakeFiles')
            os.remove(work_dir+"/CMakeCache.txt")
            os.remove(work_dir+"/cmake_install.cmake")
            os.remove(work_dir+"/Makefile")

subproc  = None
lxc      = shutil.which("lxc")
username = getpass.getuser()

if( lxc is None ):
    print("Could not find lxd in the path, please make sure it is installed")
    sys.exit(1)

def _doMapAllPaths (matchobj):
    return matchobj.group(1)+"/var/lib/lxd/containers/"+container+"/rootfs/"+matchobj.group(2)

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
    sys.exit(1)

signal.signal(signal.SIGTERM, exit_gracefully)
signal.signal(signal.SIGINT , exit_gracefully)
signal.signal(signal.SIGHUP , exit_gracefully)

#force bash shell so we can be sure the environment is set up correcly by sourcing files in /etc/profile.d
subproc = subprocess.Popen([scriptpath+"/usdk-target.py","run",container,command]+args,stdout=subprocess.PIPE, stderr=subprocess.PIPE)

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
    sys.stdout.write(str(mapPaths(stdout).encode("utf-8")))

if(len(stderr) != 0):
    sys.stderr.write(str(mapPaths(stderr).encode("utf-8")))

sys.exit(subproc.returncode)
