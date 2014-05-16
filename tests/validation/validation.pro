QT = core gui
CONFIG += c++11

#QTC_LIB_DEPENDS += utils
include(../shared.pri)

PLUGIN_SRC_ROOT = $${PWD}/../../src/ubuntu

INCLUDEPATH += $${PLUGIN_SRC_ROOT}

HEADERS += \
    $${PLUGIN_SRC_ROOT}/ubuntuvalidationresultmodel.h \
    tst_validation.cpp \
    tst_validation.h

SOURCES += \
    $${PLUGIN_SRC_ROOT}/ubuntuvalidationresultmodel.cpp \
    tst_validation.cpp

OTHER_FILES += \
    simplesection.json \
    fulloutput.json

RESOURCES += \
    resources.qrc


