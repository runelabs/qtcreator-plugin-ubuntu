QT = core gui qml

CONFIG += c++11

QTC_LIB_DEPENDS += utils
QTC_PLUGIN_DEPENDS += coreplugin cmakeprojectmanager
include(../shared.pri)

PLUGIN_SRC_ROOT = $${PWD}/../../src/ubuntu

INCLUDEPATH += $${PLUGIN_SRC_ROOT}
DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += IN_TEST_PROJECT

SOURCES += \
    tst_manifest.cpp \
    $${PLUGIN_SRC_ROOT}/ubuntuclickmanifest.cpp \
    $${PLUGIN_SRC_ROOT}/ubuntuclicktool.cpp



HEADERS += \
    tst_manifest.h \
    $${PLUGIN_SRC_ROOT}/ubuntuclickmanifest.h \
    $${PLUGIN_SRC_ROOT}/ubuntuclicktool.h

OTHER_FILES +=

RESOURCES += \
    $${PLUGIN_SRC_ROOT}/resources.qrc \
    resources_test.qrc

