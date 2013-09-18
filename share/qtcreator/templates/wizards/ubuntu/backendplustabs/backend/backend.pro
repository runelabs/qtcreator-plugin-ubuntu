TEMPLATE = subdirs
SUBDIRS = modules/%DISPLAYNAME%

OTHER_FILES += $$system(find tests -type f)

check.target = check
check.commands = qmltestrunner -import modules -platform ubuntu
check.depends = modules/%DISPLAYNAME%
QMAKE_EXTRA_TARGETS += check

