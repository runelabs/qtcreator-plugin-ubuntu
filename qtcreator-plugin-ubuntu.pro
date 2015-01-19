TEMPLATE = subdirs
SUBDIRS = src/ubuntu \
          tests \
          chroot-agent

OTHER_FILES +=  \
    ubuntu-click.prf \
    ubuntu-click-tools.prf \
    share/qtcreator/ubuntu/scripts/*.py

qt_install_libs = $$[QT_INSTALL_LIBS]
QMAKE_INST_EXTRA_FILES.path=/lib/$$basename(qt_install_libs)/qt5/mkspecs/features
QMAKE_INST_EXTRA_FILES.files= ubuntu-click.prf \
                              ubuntu-click-tools.prf

INSTALLS+=QMAKE_INST_EXTRA_FILES
