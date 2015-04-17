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


SOURCES += main.cpp \
    chrootagent.cpp

HEADERS += \
    chrootagent.h

xml_desc.target=com.ubuntu.sdk.ClickChrootAgent.xml
xml_desc.commands=qdbuscpp2xml -o $$xml_desc.target $$PWD/chrootagent.h
xml_desc.depends=$$PWD/chrootagent.h

QMAKE_EXTRA_TARGETS+=xml_desc

DBUS_ADAPTORS += $$xml_desc.target
DBUS_INTERFACES += $$xml_desc.target

target.path=/bin
INSTALLS+=target
