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

    BUILD_ROOT_STR = '\\"$$clean_path($${OUT_PWD}/../../)\\"'
    DEFINES += UBUNTU_BUILD_ROOT=\"$${BUILD_ROOT_STR}\"

    #create a link so we get our wizards in the new project wizard
    system("rm '$$(HOME)/.config/QtProject/qtcreator/templates'")
    system("ln -s '$${PWD}/../share/qtcreator/templates' '$$(HOME)/.config/QtProject/qtcreator/templates'")
}


include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)

INCLUDEPATH += $$QTCREATOR_SOURCES/src/

## make sure the QtProject libs are available when building locally
!isEmpty(UBUNTU_LOCAL_BUILD) {
    LIBS += -L$$DESTDIRBASE/QtProject/$$DESTDIRAPPNAME/plugins/$$QTCREATOR_VERSION/QtProject
}
LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator
LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator/plugins/QtProject
