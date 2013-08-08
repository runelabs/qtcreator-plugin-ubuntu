TEMPLATE = lib
TARGET = %DISPLAYNAME%
QT += qml quick
CONFIG += qt plugin no_keywords

#comment in the following line to disable traces
DEFINES += QT_NO_DEBUG_OUTPUT

TARGET = $$qtLibraryTarget($$TARGET)
uri = %DISPLAYNAME%

INCLUDEPATH += .

# Input
HEADERS += backend.h mytype.h
SOURCES += backend.cpp mytype.cpp

# Install path for the plugin
installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)
target.path = $$installPath
INSTALLS += target

# find files
QMLDIR_FILE = qmldir

# make visible to qt creator
OTHER_FILES += $$QMLDIR_FILE 

# create install targets for files
qmldir.path = $$installPath
qmldir.files = $$QMLDIR_FILE

INSTALLS += qmldir

