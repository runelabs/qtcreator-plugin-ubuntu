TEMPLATE = subdirs
SUBDIRS = src/ubuntu \
          tests \
          chroot-agent

#QML files
QML_ROOT="$${PWD}/share/qtcreator/ubuntu"
QML_FILES += \
    $$QML_ROOT/qml/Components/Link.qml \
    $$QML_ROOT/qml/Components/NewsBox.qml\
    $$QML_ROOT/qml/Components/FeatureStateItem.qml  \
    $$QML_ROOT/qml/Components/ScrollableView.qml \
    $$QML_ROOT/qml/Components/SectionItem.qml \
    $$QML_ROOT/qml/Components/Link.qml \
    $$QML_ROOT/qml/DevicesPage/DevicePage.qml  \
    $$QML_ROOT/qml/DevicesPage/NewEmulatorDialog.qml \
    $$QML_ROOT/qml/DevicesPage/DeleteDeviceDialog.qml \
    $$QML_ROOT/qml/DevicesPage/EmulatorNotInstalled.qml \
    $$QML_ROOT/qml/welcome.qml \
    $$QML_ROOT/qml/publishpage.qml \
    $$QML_ROOT/qml/devicespage.qml


OTHER_FILES +=  \
    ubuntu-click.prf \
    ubuntu-click-tools.prf \
    share/qtcreator/ubuntu/scripts/*.py \
    $$QML_FILES

qt_install_libs = $$[QT_INSTALL_LIBS]
QMAKE_INST_EXTRA_FILES.path=/lib/$$basename(qt_install_libs)/qt5/mkspecs/features
QMAKE_INST_EXTRA_FILES.files= ubuntu-click.prf \
                              ubuntu-click-tools.prf

INSTALLS+=QMAKE_INST_EXTRA_FILES
