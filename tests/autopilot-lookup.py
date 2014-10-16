#!/usr/bin/env python

from autopilot.introspection import get_proxy_object_for_existing_process
from autopilot.introspection.dbus import StateNotFoundError
import sys
import subprocess
import re

if __name__ == '__main__':

    def findInnermostChild (objectlist, x,y):
        for child in objectlist:
            try:
                object_rect = child.globalRect
            except AttributeError:
                #object does not have a rect but maybe the childs?
                children = child.get_children()
                if (len(children) > 0):
                    findInnermostChild(children,x,y)

                continue
            except StateNotFoundError:
                print "StateNotFoundError"
                return

            x1 = int(object_rect.x)
            x2 = int(x1 + object_rect.width)
            y1 = int(object_rect.y)
            y2 = int(y1 + object_rect.height)

            if (x >= x1 and x <= x2 and y >= y1 and y <= y2):
                children = child.get_children()
                if (len(children) > 0):
                    findInnermostChild(children,x,y)
                else:
                    child.print_tree(output=sys.stdout, maxdepth=1, _curdepth=0)

    if len(sys.argv) < 2:
        print "Error: please supply executable"
        sys.exit(1)
    app_exec = str(sys.argv[1])

    introspec_obj=None
    proc = subprocess.Popen([app_exec,'-testability'],stderr=subprocess.PIPE)
    while True:
        line = proc.stderr.readline()
        if not line:
            break;

        if not introspec_obj:
            retry=0
            while(retry < 5):
                print("Trying to get the Proxy object")
                try:
                    introspec_obj = get_proxy_object_for_existing_process(pid=proc.pid)
                    print("Done")
                    break
                except:
                    retry+=1


        match = re.match(r"MOUSEDOWN\s+([0-9]+):([0-9]+)",line)
        if match:
            print ("MouseDown:", match.group(1),match.group(2))
            print ("Searching innermost object")
            findInnermostChild(introspec_obj.get_children(),int(match.group(1)),int(match.group(2)))
            print ("Done")
