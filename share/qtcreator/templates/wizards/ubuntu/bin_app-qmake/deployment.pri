qt_install_libs = $$[QT_INSTALL_LIBS]
target.path = /lib/$$basename(qt_install_libs)/bin
export(target.path)
INSTALLS += target

