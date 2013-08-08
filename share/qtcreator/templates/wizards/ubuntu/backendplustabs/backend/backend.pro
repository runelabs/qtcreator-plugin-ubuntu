TEMPLATE = subdirs
SUBDIRS = modules/%DISPLAYNAME%

OTHER_FILES += $$system(find tests -type f)

check.target = check
check.commands = qmltestrunner -import modules
check.depends = modules/%DISPLAYNAME%
QMAKE_EXTRA_TARGETS += check

