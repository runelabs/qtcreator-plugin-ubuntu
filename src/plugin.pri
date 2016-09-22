PROVIDER = Canonical

#support compiling inside the QtC source tree
exists( $$PWD/../../plugins.pro ) {
    message( "Building ubuntu plugin as part of QtCreator" )
    include($$PWD/../../../qtcreatorplugin.pri)

    USDK_DATA_DIRS = \
        $$PWD/../share/qtcreator/templates \
        $$PWD/../share/qtcreator/ubuntu \

    for(data_dir, USDK_DATA_DIRS) {
        files = $$files($$data_dir/*, true)
        # Info.plist.in are handled below
        for(file, files):!exists($$file/*): {
            USDK_FILES += $$file
        }
    }

    #seems $$files can not find hidden files ---
    USDK_FILES += $$PWD/../share/qtcreator/templates/wizards/ubuntu/share/.excludes

    ubuntusdk_copy2build.input = USDK_FILES
    ubuntusdk_copy2build.output = $$IDE_DATA_PATH/../../${QMAKE_FUNC_FILE_IN_stripSrcDir}
    ubuntusdk_copy2build.variable_out = PRE_TARGETDEPS
    ubuntusdk_copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
    ubuntusdk_copy2build.name = COPY ${QMAKE_FILE_IN}
    ubuntusdk_copy2build.CONFIG += no_link
    QMAKE_EXTRA_COMPILERS += ubuntusdk_copy2build

    for(data_dir, USDK_DATA_DIRS) {
        eval($${data_dir}.files = $$IDE_DATA_PATH/../../$$stripSrcDir($$data_dir))
        eval($${data_dir}.path = $$QTC_PREFIX/share/qtcreator)
        eval($${data_dir}.CONFIG += no_check_exist)
        INSTALLS += $$data_dir
    }
} else {
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
    LIBS += -L$$[QT_INSTALL_LIBS]/qtcreator/plugins
}

