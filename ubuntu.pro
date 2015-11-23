TEMPLATE = subdirs
SUBDIRS = UbuntuPlugin.pro \
          #tests \
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
    $$QML_ROOT/qml/Components/UbuntuStyleToolbar.qml \
    $$QML_ROOT/qml/DevicesPage/DevicePage.qml  \
    $$QML_ROOT/qml/DevicesPage/NewEmulatorDialog.qml \
    $$QML_ROOT/qml/DevicesPage/DeleteDeviceDialog.qml \
    $$QML_ROOT/qml/DevicesPage/EmulatorNotInstalled.qml \
    $$QML_ROOT/qml/welcome.qml \
    $$QML_ROOT/qml/publishpage.qml \
    $$QML_ROOT/qml/devicespage.qml

OTHER_FILES +=  \
    share/qtcreator/ubuntu/scripts/*.py \
    $$QML_FILES

#support compiling inside the QtC source tree
!exists( $$PWD/../plugins.pro ) {
    qt_install_libs = $$[QT_INSTALL_LIBS]
}

