TEMPLATE = app
TARGET = %ClickHookName%

QT += qml quick

SOURCES += main.cpp

RESOURCES += %ClickHookName%.qrc

OTHER_FILES += %ClickHookName%.apparmor \
               %ClickHookName%.desktop \
               %ClickHookName%.png

#specify where the config files are installed to
config_files.path = /%ClickHookName%
config_files.files += $${OTHER_FILES}
message($$config_files.files)
INSTALLS+=config_files

# Default rules for deployment.
include(../deployment.pri)
