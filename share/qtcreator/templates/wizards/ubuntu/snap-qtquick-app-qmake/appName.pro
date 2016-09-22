# This is the basic qmake template for the Ubuntu-SDK
# it handles creation and installation of the manifest
# file and takes care of subprojects
TEMPLATE = subdirs

SUBDIRS += %{ProjectNameL}

# enables/disabled the extra targets to build a snapcraft package
# also tells the IDE this is a snapcraft project
CONFIG += snapcraft

snapcraft {

    SNAPCRAFT_FILE=snapcraft.yaml

    #the Ubuntu SDK IDE uses the snap target to create the package
    snappy.target = snap
    snappy.commands = cd $$OUT_PWD
    snappy.commands += && rm -rf \'$$OUT_PWD/snap-deploy\'
    snappy.commands += && make INSTALL_ROOT=$$OUT_PWD/snap-deploy install
    snappy.commands += && cd $$OUT_PWD/snap-deploy
    snappy.commands += && snapcraft

    OTHER_FILES+=$$SNAPCRAFT_FILE
    QMAKE_EXTRA_TARGETS += snappy

    packaging.files = $$SNAPCRAFT_FILE
    packaging.path  = /

    INSTALLS+=packaging
}

