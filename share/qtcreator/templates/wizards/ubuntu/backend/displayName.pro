TEMPLATE = subdirs
SUBDIRS = modules/%DISPLAYNAME%

check.target = check
check.commands = qmltestrunner -import modules -platform ubuntu
check.depends = modules/%DISPLAYNAME%
QMAKE_EXTRA_TARGETS += check

