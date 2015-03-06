PROVIDER = Canonical

RELEASE_BUILD=1
SCRIPTS_FROM_SRC=0
SET_BUILD_ROOT=0
LINK_TEMPLATES=0
COPY_DATAFILES=0

## Where the Qt Creator headers are located at
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=/usr/src/qtcreator

## Where our plugin will be compiled to
IDE_BUILD_TREE = $$(QTC_BUILD)

#support for compiling INTO a locally checked out QtC source tree
UBUNTU_QTC_PLUGIN_QTCTREE_BUILD=$$(UBUNTU_QTC_PLUGIN_QTCTREE_BUILD)
!isEmpty(UBUNTU_QTC_PLUGIN_QTCTREE_BUILD): {
    QTCREATOR_SOURCES=$$clean_path($${UBUNTU_QTC_PLUGIN_QTCTREE_BUILD})

    #IDE_BUILD_TREE needs to be set to make this work properly
    isEmpty(IDE_BUILD_TREE): error("QTC_BUILD needs to point to the QtCreator build directory!")

    RELEASE_BUILD=0
    SCRIPTS_FROM_SRC=1
    SET_BUILD_ROOT=1
    COPY_DATAFILES=1
}

#support for compiling into the user plugin directory
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

isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=../../builddir
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)


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

    equals(COPY_DATAFILES, 1) {
        USDK_DATA_DIRS = \
            ../share/qtcreator/templates \
            ../share/qtcreator/ubuntu \

        for(data_dir, USDK_DATA_DIRS) {
            files = $$files($$PWD/$$data_dir/*, true)
            # Info.plist.in are handled below
            for(file, files):!exists($$file/*): USDK_FILES += $$file

        }
        ubuntusdk_copy2build.input = USDK_FILES
        ubuntusdk_copy2build.output = $$IDE_DATA_PATH/${QMAKE_FUNC_FILE_IN_stripSrcDir}
        ubuntusdk_copy2build.variable_out = PRE_TARGETDEPS
        ubuntusdk_copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
        ubuntusdk_copy2build.name = COPY ${QMAKE_FILE_IN}
        ubuntusdk_copy2build.CONFIG += no_link
        QMAKE_EXTRA_COMPILERS += ubuntusdk_copy2build
    }
} else {
    LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator
    LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator/plugins/QtProject
}
