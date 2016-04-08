#!/usr/bin/env python3
# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# QTC device applauncher
# Copyright (C) 2016 Canonical
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

import argparse
import json
import sys
import shutil
import shlex
import getpass
import os
import pylxd
import stat
import subprocess
from pylxd.deprecated.exceptions import PyLXDException

import pwd
import spwd
import grp

def bootOrDie (apiObj, container):
    if not apiObj.container_defined(container):
        print("Container not found")
        sys.exit(1)
        
    info = apiObj.container_info(container)
    if info['status'] != "Running":    
        op = pylxd.api.operation.LXDOperation();
        op_data = apiObj.container_start(container, -1)
        if not op.operation_wait(op_data[1]["operation"], op_data[1]["status_code"], -1):
            print("Could not start container")
            sys.exit(1)

def findExistingTargets ():
    apiObj = pylxd.api.container.LXDContainer()
    containers = apiObj.container_list()
    targets = []
    for container in containers:
        config = apiObj.get_container_config(container)
        if ("config" in config 
        and "user.click-architecture" in config["config"]
        and "user.click-framework" in config["config"]):
            targets.append(dict(
                name= container,
                architecture= config["config"]["user.click-architecture"],
                framework= config["config"]["user.click-framework"]
            ))
    return targets;

def executeCommand (args, priviledged):
    apiObj = pylxd.api.API()
    bootOrDie(apiObj, args.name)
    
    lxc_command = shutil.which("lxc")
    if lxc_command is None:
        print("lxc was not found in PATH")
        sys.exit(1)
        
    lxc_args   = []
    lxc_args.append(lxc_command)
    lxc_args.append("exec")
    lxc_args.append(args.name)
    lxc_args.append("--")
    lxc_args.append("su")
    lxc_args.append("-l")
    lxc_args.append("-s")
    lxc_args.append("/bin/bash")
    
    if priviledged == False:
        lxc_args.append(getpass.getuser())
    
    if args.program:
        program = "cd \""+os.getcwd()+"\" && "
        program +=" LC_ALL=C "
        program += " ".join(shlex.quote(arg) for arg in args.program)
        lxc_args.append("-c")
        lxc_args.append(program)
        
    sys.stdout.flush()
    sys.stderr.flush()
    os.execv(lxc_command, lxc_args)
    
def containerBasePath (name):
    apiObj = pylxd.api.API()
    if apiObj.container_defined(name):
        return "/var/lib/lxd/containers/"+name
    return None
    
def containerRootFs (name):
    basePath = containerBasePath(name)
    if basePath is None:
        return None
    return basePath+"/rootfs"
    
def statusCmd(args):
    apiObj = pylxd.api.API()
    bootOrDie(apiObj, args.name)
    
    info = apiObj.container_info(args.name)
    status = {}
    status["status"] = info["status"]
    
    if "network" in info and "eth0" in info["network"]:
        if "addresses" in info["network"]["eth0"]:
            for add in info["network"]["eth0"]["addresses"]:
                if add["family"] == "inet":
                    status["ipv4"] = add["address"]
                    break
    
    print(json.dumps(status))
    

def listCmd (args):
    print(json.dumps(findExistingTargets()))
    
def existsCmd (args):
    apiObj = pylxd.api.API()
    if apiObj.container_defined(args.name):
        print("Container exists") 
        sys.exit(0)
    print("Container not found")
    sys.exit(1)
    
def createCmd (args):
    if os.getuid() is not 0:
        print ("Must be root to create a new container")
        sys.exit(1)
    
    imagename = args.framework+"-"+args.architecture
    print("Creating builder target from image: "+imagename)
    apiObj = pylxd.api.API()
    if apiObj.container_defined(args.name):
        print("Container already exists")
        sys.exit(1)
        
    #todo check if host arch can run the container arch
    config = {
          "security.privileged":"true",
          "user.click-architecture":"armhf",
          "user.click-framework":"ubuntu-sdk-15.04"
    }
    
    devices = {
         "dri":{  
            "path":"/dev/dri",
            "source":"/dev/dri",
            "type":"disk"
         },
         "homedir":{  
            "path":"/home",
            "source":"/home",
            "type":"disk"
         },
         "root":{  
            "path":"/",
            "type":"disk"
         },
         "tmp":{  
            "path":"/tmp",
            "source":"/tmp",
            "type":"disk"
         }
    }
    
    container = {
      'name': args.name,
      'source': {
         'type': 'image',
         'alias': imagename
      },
      'config':config, 
      'devices':devices             
    }
    
    op = pylxd.api.operation.LXDOperation();
    try:
        op_data = apiObj.container_init(container)
        if op.operation_wait(op_data[1]["operation"], op_data[1]["status_code"], -1):
            basePath = containerBasePath(args.name)
            if (os.path.islink(basePath)):
                basePath = os.path.realpath(basePath)
                
            os.chmod(basePath, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)            
            registerCmd((argparse.Namespace(name=args.name, user=None)))
            sys.exit(0)
        else:
            print("Creating the container failed")
            sys.exit(1)
    except PyLXDException as ex:
        print("Creating the container failed with error: "+str(ex))
        sys.exit(1)
        
def destroyCmd (args):
    apiObj = pylxd.api.API()
    if not apiObj.container_defined(args.name):
        print("Container does not exists")
        sys.exit(1)
        
    op = pylxd.api.operation.LXDOperation();
    try:
        op_data = apiObj.container_destroy(args.name)
        if op.operation_wait(op_data[1]["operation"], op_data[1]["status_code"], -1):
            print("Success")
            sys.exit(0)
        else:
            print("Removing the container failed")
            sys.exit(1)
    except PyLXDException as ex:
        print("Removing the container failed with error: "+str(ex))
        sys.exit(1)
    
def registerCmd (args):
    if os.getuid() is not 0:
        print ("Must be root to register new users inside the container")
        sys.exit(1)
        
    apiObj = pylxd.api.API()
    bootOrDie(apiObj, args.name)
    
    user_entry = None
    if args.user is None:
        env = os.environ
        if "SUDO_UID" in env:
            user_entry = pwd.getpwuid(int(env["SUDO_UID"]))
        elif "PKEXEC_UID" in env:
            user_entry = pwd.getpwuid(int(env["PKEXEC_UID"]))
    else:
        user_entry   = pwd.getpwnam(args.user)
        
    if user_entry is None:
        print("Can not determine user, please specify with the --user switch")
        sys.exit(1)
        
    if user_entry[2] == 0:
        print ("Can not register root.")
        sys.exit(1)
        
    shadow_entry = spwd.getspnam(user_entry[0])
    
    groups=[]
    print(grp.getgrall())
    for currGrp in grp.getgrall():
        
        if user_entry[0] in currGrp[3] or currGrp[0] == user_entry[0]:
            
            is_primGroup = (currGrp[2] == user_entry[3])
            
            print ("Create group "+currGrp[0])
            
            #autocreate groups
            cmd = ["lxc", "exec", args.name, "--",
                   "groupadd", 
                   "-g", str(currGrp[2]),
                   currGrp[0]]
            
            res = subprocess.call(cmd)
            # 0=success
            # 9=group name already exists
            if res is not 0 and res is not 9:
                if not is_primGroup:
                    print("Could not create group "+currGrp[0]+" skipping it")
                    continue;
                else:
                    print("Could not create primary group")
                    sys.exit(1)
            
            # we don't need the default group in the supplementary ones
            if not is_primGroup:
                groups.append(currGrp[0])
            
            
    print ("Adding user")
    cmd = ["lxc", "exec", args.name, "--",
           "useradd", "--no-create-home", 
           "-u", str(user_entry[2]),
           "--gid", str(user_entry[3]), 
           "--groups", ",".join(groups),
           "--home-dir", user_entry[5],
           "-s", "/bin/bash",
           "-p", shadow_entry[1],
           user_entry[0]]
    
    print(" ".join(cmd))
            
    sys.exit(subprocess.call(cmd))
    
def rootfsCmd (args):
    #todo: ask the container where the filesystem is
    rootfs = containerRootFs(args.name)
    if rootfs is None:
        print("Container not found")
        sys.exit(1)
        
    print(rootfs)
    sys.exit(0)
    
def runCmd (args):
    executeCommand(args, False)
    
def maintCmd (args):
    print (args)
    executeCommand(args, True)
    
def upgradeCmd (args):
    maintCmd(argparse.Namespace(name=args.name, program=["/bin/bash", "-c", "apt update && apt full-upgrade --yes"]))

# register options to the argument parser
parser = argparse.ArgumentParser(description="Ubuntu SDK build target utility")
subparsers = parser.add_subparsers()

list_parser = subparsers.add_parser("list")
list_parser.set_defaults(func=listCmd)

rootfs_parser = subparsers.add_parser("rootfs")
rootfs_parser.add_argument("name", action="store")
rootfs_parser.set_defaults(func=rootfsCmd)

status_parser = subparsers.add_parser("status")
status_parser.add_argument("name", action="store")
status_parser.set_defaults(func=statusCmd)

exists_parser = subparsers.add_parser("exists")
exists_parser.add_argument("name", action="store")
exists_parser.set_defaults(func=existsCmd)

upgrade_parser = subparsers.add_parser("upgrade")
upgrade_parser.add_argument("name", action="store")
upgrade_parser.set_defaults(func=upgradeCmd)

run_parser = subparsers.add_parser("run")
run_parser.add_argument("name", action="store")
run_parser.add_argument(
        "program", nargs=argparse.REMAINDER,
        help="program to run with arguments")
run_parser.set_defaults(func=runCmd)

maint_parser = subparsers.add_parser("maint",
    help="run a maintenance command in the container")
maint_parser.add_argument("name", action="store")
maint_parser.add_argument("program", 
    nargs=argparse.REMAINDER,
    help="program to run with arguments")
maint_parser.set_defaults(func=maintCmd)

create_parser = subparsers.add_parser("create")
create_parser.set_defaults(func=createCmd)
create_parser.add_argument(
    "-a", "--architecture", required=True,
    help="architecture for the chroot")
create_parser.add_argument(
    "-f", "--framework", default="ubuntu-sdk-15.04",
    help="framework for the chroot (default: ubuntu-sdk-15.04)")
create_parser.add_argument(
    "-n", "--name", required=True,
    help=(
        "name of the container"))

register_parser = subparsers.add_parser("register", help="register a user into the target")
register_parser.add_argument("name", action="store")
register_parser.add_argument("-u", "--user", required=False, default=None)
register_parser.set_defaults(func=registerCmd)

destroy_parser = subparsers.add_parser("destroy")
destroy_parser.add_argument("name", action="store")
destroy_parser.set_defaults(func=destroyCmd)

options = parser.parse_args()
options.func(options)