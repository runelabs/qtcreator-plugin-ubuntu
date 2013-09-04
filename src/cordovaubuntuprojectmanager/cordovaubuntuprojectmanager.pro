TARGET = CordovaUbuntuProjectManager
TEMPLATE = lib

DEFINES += CORDOVAUBUNTUPROJECTMANAGER_LIBRARY

QMAKE_CXXFLAGS += -std=c++0x -Wall -Wextra

SOURCES += \
    plugin.cpp \
    cproject.cpp \
    cprojectfile.cpp \
    cruncontrol.cpp \
    cprojectnode.cpp \
    crunconfiguration.cpp

HEADERS += \
    constants.h \
    global.h \
    plugin.h \
    common.h \
    cprojectmanager.h \
    cproject.h \
    cprojectfile.h \
    cruncontrol.h \
    cprojectnode.h \
    crunconfiguration.h

PROVIDER = Canonical

## Where the Qt Creator headers are located at
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=/usr/src/qtcreator

## Where our plugin will be compiled to
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=../../builddir

include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
include($$QTCREATOR_SOURCES/src/plugins/coreplugin/coreplugin.pri)
include($$QTCREATOR_SOURCES/src/plugins/projectexplorer/projectexplorer.pri)
include($$QTCREATOR_SOURCES/src/plugins/debugger/debugger.pri)
include(../ubuntu/ubuntu.pri)

INCLUDEPATH += $$QTCREATOR_SOURCES/src
LIBS += -L/usr/lib/qtcreator
LIBS += -L/usr/lib/qtcreator/plugins/QtProject

OTHER_FILES += CordovaUbuntuProject.mimetypes.xml

RESOURCES += cordovaubuntuproject.qrc

