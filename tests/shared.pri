PROVIDER = Canonical

## Where the Qt Creator headers are located at
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=/usr/src/qtcreator

## Where our plugin will be compiled to
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=../../builddir

UBUNTU_LOCAL_BUILD = $$(UBUNTU_QTC_PLUGIN_LOCALBUILD)
!isEmpty(UBUNTU_LOCAL_BUILD) {
    message("!!!!!!!!!!BUILDING LOCAL VERSION OF PLUGIN !!!!!!!!!!!!!!!!!!!")
    USE_USER_DESTDIR = yes
    PATHSTR = '\\"$${PWD}/../share/qtcreator\\"'

    DEFINES += UBUNTU_RESOURCE_PATH_LOCAL=\"$${PATHSTR}\" UBUNTU_BUILD_LOCAL
}


include($$QTCREATOR_SOURCES/qtcreator.pri)

isEmpty(TEMPLATE):TEMPLATE=app
QT += testlib
CONFIG += qt warn_on console depend_includepath testcase
CONFIG -= app_bundle

DEFINES -= QT_NO_CAST_FROM_ASCII
# prefix test binary with tst_
!contains(TARGET, ^tst_.*):TARGET = $$join(TARGET,,"tst_")

QMAKE_RPATHDIR += $$IDE_BUILD_TREE/$$IDE_LIBRARY_BASENAME/qtcreator
QMAKE_RPATHDIR += $$IDE_PLUGIN_PATH/QtProject
QMAKE_RPATHDIR += $$IDE_PLUGIN_PATH/Canonical

IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")

QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'

INCLUDEPATH += $$QTCREATOR_SOURCES/src/

## make sure the QtProject libs are available when building locally
!isEmpty(UBUNTU_LOCAL_BUILD) {

    DESTDIRAPPNAME = "qtcreator"
    DESTDIRBASE = "$$(XDG_DATA_HOME)"
    isEmpty(DESTDIRBASE):DESTDIRBASE = "$$(HOME)/.local/share/data"
    else:DESTDIRBASE = "$$DESTDIRBASE/data"

    LIBS += -L$$DESTDIRBASE/QtProject/$$DESTDIRAPPNAME/plugins/$$QTCREATOR_VERSION/QtProject
    LIBS += -L$$DESTDIRBASE/QtProject/$$DESTDIRAPPNAME/plugins/$$QTCREATOR_VERSION/Canonical
}
LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator
LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator/plugins/QtProject
LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator/plugins/Canonical
