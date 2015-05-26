#-------------------------------------------------
#
# Project created by QtCreator 2014-12-09T14:24:45
#
#-------------------------------------------------

QT       += core dbus

QT       -= gui

TARGET    = click-chroot-agent
CONFIG   += console
CONFIG   -= app_bundle

QMAKE_CXXFLAGS += -Werror
CONFIG += c++11 dbusadaptors dbusinterfaces

TEMPLATE = app

#support compiling inside the QtC source tree
exists( $$PWD/../../plugins.pro ) {
    include(../../../../qtcreator.pri)
    target.path=$$IDE_LIBEXEC_PATH/bin
} else {

    ## Where the Qt Creator headers are located at
    QTCREATOR_SOURCES = $$(QTC_SOURCE)
    isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=/usr/src/qtcreator

    ## Where our plugin will be compiled to
    IDE_BUILD_TREE = $$(QTC_BUILD)
    isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=../../builddir

    include($$QTCREATOR_SOURCES/qtcreator.pri)

    target.path=/bin
}

SOURCES += main.cpp \
    chrootagent.cpp

HEADERS += \
    chrootagent.h

INCLUDEPATH += $$OUT_PWD

xml_desc.target=com.ubuntu.sdk.ClickChrootAgent.xml
xml_desc.commands=$$[QT_INSTALL_BINS]/qdbuscpp2xml -o $$xml_desc.target $$PWD/chrootagent.h
xml_desc.depends=$$PWD/chrootagent.h

QMAKE_EXTRA_TARGETS+=xml_desc

DBUS_ADAPTORS += $$xml_desc.target
DBUS_INTERFACES += $$xml_desc.target


INSTALLS+=target
