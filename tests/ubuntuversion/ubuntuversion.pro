QT = core gui
CONFIG += c++11

QTC_LIB_DEPENDS += utils
QTC_PLUGIN_DEPENDS += coreplugin
include(../shared.pri)

PLUGIN_SRC_ROOT = $${PWD}/../../src/ubuntu

INCLUDEPATH += $${PLUGIN_SRC_ROOT}
DEFINES += SRCDIR=\\\"$$PWD/\\\"

SOURCES += tst_ubuntuversiontest.cpp \
    $${PLUGIN_SRC_ROOT}/ubuntuversion.cpp

HEADERS += \
    tst_ubuntuversiontest.h \
    $${PLUGIN_SRC_ROOT}/ubuntuversion.h

OTHER_FILES +=

RESOURCES += \
    resources.qrc
