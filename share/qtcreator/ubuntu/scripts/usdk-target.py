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
from pylxd import api

def findExistingTargets ():
    apiObj = api.container.LXDContainer()
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
    

def listCmd (args):
    print(json.dumps(findExistingTargets()))
    
def existsCmd (args):
    containers = findExistingTargets()
    for container in containers:
        if (container["name"] == args.name):
            print("Container exists") 
            sys.exit(0)
    print("Container not found")
    sys.exit(1)
    
def createCmd (args):
    print("Create")
    
def registerCmd (args):
    print("Register")
    
def rootfsCmd (args):
    #todo: ask the container where the filesystem is
    print("/var/lib/lxd/containers/"+args.name+"/rootfs")
    sys.exit(0)
    


# register options to the argument parser
parser = argparse.ArgumentParser(description="Ubuntu SDK build target utility")
subparsers = parser.add_subparsers()

list_parser = subparsers.add_parser("list")
list_parser.set_defaults(func=listCmd)

rootfs_parser = subparsers.add_parser("rootfs")
rootfs_parser.add_argument("name", action="store")
rootfs_parser.set_defaults(func=rootfsCmd)

exists_parser = subparsers.add_parser("exists")
exists_parser.add_argument("name", action="store")
exists_parser.set_defaults(func=existsCmd)

create_parser = subparsers.add_parser("create")
create_parser.set_defaults(func=createCmd)

register_parser = subparsers.add_parser("register")
register_parser.set_defaults(func=registerCmd)

options = parser.parse_args()
options.func(options)