PROVIDER = Canonical

RELEASE_BUILD=1
SCRIPTS_FROM_SRC=0
SET_BUILD_ROOT=0
LINK_TEMPLATES=0

## Where the Qt Creator headers are located at
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=/usr/src/qtcreator

## Where our plugin will be compiled to
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=../../builddir

UBUNTU_QTC_PLUGIN_QTCTREE_BUILD=$$(UBUNTU_QTC_PLUGIN_QTCTREE_BUILD)
!isEmpty(UBUNTU_QTC_PLUGIN_QTCTREE_BUILD): {
    RELEASE_BUILD=0
    SCRIPTS_FROM_SRC=1
    SET_BUILD_ROOT=1

    QTCREATOR_SOURCES=$$clean_path($${UBUNTU_QTC_PLUGIN_QTCTREE_BUILD})

    message($$QTCREATOR_SOURCES)
}

UBUNTU_LOCAL_BUILD = $$(UBUNTU_QTC_PLUGIN_LOCALBUILD)
!isEmpty(UBUNTU_LOCAL_BUILD) {
    RELEASE_BUILD=0
    SCRIPTS_FROM_SRC=1
    SET_BUILD_ROOT=1
    LINK_TEMPLATES=1

    USE_USER_DESTDIR = yes

    ## make sure the QtProject libs are available when building locally
    LIBS += -L$$DESTDIRBASE/QtProject/$$DESTDIRAPPNAME/plugins/$$QTCREATOR_VERSION/QtProject
}

equals(RELEASE_BUILD, 0) {

    equals(SCRIPTS_FROM_SRC, 1) {
        PATHSTR = '\\"$${PWD}/../share/qtcreator\\"'
        DEFINES += UBUNTU_RESOURCE_PATH_LOCAL=\"$${PATHSTR}\" UBUNTU_BUILD_LOCAL
    }

    equals(SET_BUILD_ROOT, 1) {
        BUILD_ROOT_STR = '\\"$$clean_path($${OUT_PWD}/../../)\\"'
        DEFINES += UBUNTU_BUILD_ROOT=\"$${BUILD_ROOT_STR}\"
    }

    equals(LINK_TEMPLATES, 1) {
        #create a link so we get our wizards in the new project wizard
        system("rm '$$(HOME)/.config/QtProject/qtcreator/templates'")
        system("ln -s '$${PWD}/../share/qtcreator/templates' '$$(HOME)/.config/QtProject/qtcreator/templates'")
    }
}

include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)

INCLUDEPATH += $$QTCREATOR_SOURCES/src/
LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator
LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator/plugins/QtProject
