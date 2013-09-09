QMAKE_CXXFLAGS += -std=c++0x -Wall -Wextra

include(../plugin.pri)
include(../ubuntu/ubuntu.pri)

OTHER_FILES += CordovaUbuntuProject.mimetypes.xml

RESOURCES += cordovaubuntuproject.qrc

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

