QT = core gui
CONFIG += c++11

QTC_LIB_DEPENDS += utils
include(../shared.pri)

PLUGIN_SRC_ROOT = $${PWD}/../../src/ubuntu

INCLUDEPATH += $${PLUGIN_SRC_ROOT}

HEADERS += \
    $${PLUGIN_SRC_ROOT}/localportsmanager.h\
    tst_localportmanager.h

SOURCES += \
    $${PLUGIN_SRC_ROOT}/localportsmanager.cpp \
    tst_localportmanager.cpp

OTHER_FILES += simple_list

RESOURCES += \
    resources.qrc


