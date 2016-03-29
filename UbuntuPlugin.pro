QT += network qml quick dbus

include(src/plugin.pri)

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O1

QMAKE_CXXFLAGS += -Werror
CONFIG += c++11 dbusinterfaces

#####################################
# required for Ubuntu Device Notifier
CONFIG += link_pkgconfig

PKGCONFIG += libudev glib-2.0
#####################################

#####################################
## Project files

FORMS += \
    src/ubuntu/ubuntupackagingwidget.ui \
    src/ubuntu/ubuntusecuritypolicypickerdialog.ui \
    src/ubuntu/ubuntusettingsdeviceconnectivitywidget.ui \
    src/ubuntu/ubuntusettingsclickwidget.ui \
    src/ubuntu/ubuntuclickdialog.ui \
    src/ubuntu/ubuntucreatenewchrootdialog.ui \
    src/ubuntu/ubuntupackagestepconfigwidget.ui \
    src/ubuntu/ubuntumanifesteditor.ui \
    src/ubuntu/ubuntuapparmoreditor.ui \
    src/ubuntu/targetupgrademanagerdialog.ui \
    src/ubuntu/ubuntusettingsprojectdefaultspage.ui

RESOURCES += \
    src/ubuntu/resources.qrc

OTHER_FILES += \
    src/ubuntu/UbuntuProject.mimetypes.xml \
    src/ubuntu/manifest.json.template \
    src/ubuntu/myapp.json.template \
    src/ubuntu/manifestlib.js

SOURCES += \
    src/ubuntu/ubuntuplugin.cpp \
    src/ubuntu/ubuntuwelcomemode.cpp \
    src/ubuntu/wizards/ubuntuprojectapplicationwizard.cpp \
    src/ubuntu/ubuntumenu.cpp \
    src/ubuntu/ubuntuprojectmanager.cpp \
    src/ubuntu/ubuntuproject.cpp \
    src/ubuntu/ubuntuprojectfile.cpp \
    src/ubuntu/ubuntuprojectnode.cpp \
    src/ubuntu/ubuntuversion.cpp \
    src/ubuntu/ubuntufeatureprovider.cpp \
    src/ubuntu/ubuntuversionmanager.cpp \
    src/ubuntu/ubuntupackagingmode.cpp \
    src/ubuntu/ubuntubzr.cpp \
    src/ubuntu/ubuntuclickmanifest.cpp \
    src/ubuntu/ubuntudevicemode.cpp \
    src/ubuntu/ubuntuprocess.cpp \
    src/ubuntu/ubuntusecuritypolicypickerdialog.cpp \
    src/ubuntu/ubuntupolicygroupmodel.cpp \
    src/ubuntu/ubuntupolicygroupinfo.cpp \
    src/ubuntu/ubuntusettingsdeviceconnectivitypage.cpp \
    src/ubuntu/ubuntusettingsdeviceconnectivitywidget.cpp \
    src/ubuntu/ubuntusettingsclickpage.cpp \
    src/ubuntu/ubuntusettingsclickwidget.cpp \
    src/ubuntu/ubuntuclicktool.cpp \
    src/ubuntu/ubuntucreatenewchrootdialog.cpp \
    src/ubuntu/ubuntuclickdialog.cpp \
    src/ubuntu/ubuntuvalidationresultmodel.cpp \
    src/ubuntu/clicktoolchain.cpp \
    src/ubuntu/ubuntukitmanager.cpp \
    src/ubuntu/localportsmanager.cpp \
    src/ubuntu/ubuntulocaldeployconfiguration.cpp \
    src/ubuntu/ubuntulocalrunconfigurationfactory.cpp \
    src/ubuntu/ubuntulocalrunconfiguration.cpp \
    src/ubuntu/ubuntudevicesmodel.cpp \
    src/ubuntu/ubuntushared.cpp \
    src/ubuntu/ubuntupackagestep.cpp \
    src/ubuntu/ubuntuqtversion.cpp \
    src/ubuntu/ubuntudeploystepfactory.cpp \
    src/ubuntu/ubuntudirectuploadstep.cpp \
    src/ubuntu/ubuntuhtmlbuildconfiguration.cpp \
    src/ubuntu/ubuntuqmlbuildconfiguration.cpp \
    src/ubuntu/wizards/ubuntufirstrunwizard.cpp \
    src/ubuntu/ubuntumanifesteditor.cpp \
    src/ubuntu/ubuntumanifesteditorwidget.cpp \
    src/ubuntu/ubuntuabstractguieditorwidget.cpp \
    src/ubuntu/ubuntuabstractguieditor.cpp \
    src/ubuntu/ubuntuabstractguieditordocument.cpp \
    src/ubuntu/ubuntuapparmoreditor.cpp \
    src/ubuntu/ubuntueditorfactory.cpp \
    src/ubuntu/ubuntucmakecache.cpp \
    src/ubuntu/ubuntutestcontrol.cpp \
    src/ubuntu/ubuntupackageoutputparser.cpp \
    src/ubuntu/ubuntuprojecthelper.cpp \
    src/ubuntu/wizards/ubuntuprojectmigrationwizard.cpp \
    src/ubuntu/targetupgrademanager.cpp \
    src/ubuntu/ubuntupackagingmodel.cpp \
    src/ubuntu/ubuntufixmanifeststep.cpp \
    src/ubuntu/wizards/ubuntufatpackagingwizard.cpp \
    src/ubuntu/ubuntulocalruncontrolfactory.cpp \
    src/ubuntu/ubuntulocalscopedebugsupport.cpp \
    src/ubuntu/ubuntusettingsprojectdefaultspage.cpp \
    src/ubuntu/settings.cpp \
    src/ubuntu/device/container/containerdevice.cpp \
    src/ubuntu/device/container/containerdevicefactory.cpp\
    src/ubuntu/device/container/containerprocesslist.cpp \
    src/ubuntu/device/container/containerdevicesignaloperation.cpp \
    src/ubuntu/device/container/containerdeviceprocess.cpp

HEADERS += \
    src/ubuntu/ubuntuplugin.h \
    src/ubuntu/ubuntu_global.h \
    src/ubuntu/ubuntuconstants.h \
    src/ubuntu/ubuntuwelcomemode.h \
    src/ubuntu/wizards/ubuntuprojectapplicationwizard.h \
    src/ubuntu/ubuntumenu.h \
    src/ubuntu/ubuntushared.h \
    src/ubuntu/ubuntuprojectmanager.h \
    src/ubuntu/ubuntuproject.h \
    src/ubuntu/ubuntuprojectfile.h \
    src/ubuntu/ubuntuprojectnode.h \
    src/ubuntu/ubuntuversion.h \
    src/ubuntu/ubuntufeatureprovider.h \
    src/ubuntu/ubuntuversionmanager.h \
    src/ubuntu/ubuntupackagingmode.h \
    src/ubuntu/ubuntubzr.h \
    src/ubuntu/ubuntuclickmanifest.h \
    src/ubuntu/ubuntudevicemode.h \
    src/ubuntu/ubuntuprocess.h \
    src/ubuntu/ubuntusecuritypolicypickerdialog.h \
    src/ubuntu/ubuntupolicygroupmodel.h \
    src/ubuntu/ubuntupolicygroupinfo.h \
    src/ubuntu/ubuntusettingsdeviceconnectivitypage.h \
    src/ubuntu/ubuntusettingsdeviceconnectivitywidget.h \
    src/ubuntu/ubuntusettingsclickpage.h \
    src/ubuntu/ubuntusettingsclickwidget.h \
    src/ubuntu/ubuntuclicktool.h \
    src/ubuntu/ubuntucreatenewchrootdialog.h \
    src/ubuntu/ubuntuclickdialog.h \
    src/ubuntu/ubuntuvalidationresultmodel.h \
    src/ubuntu/clicktoolchain.h \
    src/ubuntu/ubuntukitmanager.h \
    src/ubuntu/localportsmanager.h \
    src/ubuntu/ubuntulocaldeployconfiguration.h \
    src/ubuntu/ubuntulocalrunconfigurationfactory.h \
    src/ubuntu/ubuntulocalrunconfiguration.h \
    src/ubuntu/ubuntudevicesmodel.h \
    src/ubuntu/ubuntupackagestep.h \
    src/ubuntu/ubuntuqtversion.h \
    src/ubuntu/ubuntudeploystepfactory.h \
    src/ubuntu/ubuntudirectuploadstep.h \
    src/ubuntu/ubuntuhtmlbuildconfiguration.h \
    src/ubuntu/ubuntuqmlbuildconfiguration.h \
    src/ubuntu/wizards/ubuntufirstrunwizard.h \
    src/ubuntu/ubuntumanifesteditor.h \
    src/ubuntu/ubuntumanifesteditorwidget.h \
    src/ubuntu/ubuntuabstractguieditorwidget.h \
    src/ubuntu/ubuntuabstractguieditor.h \
    src/ubuntu/ubuntuabstractguieditordocument.h \
    src/ubuntu/ubuntuapparmoreditor.h \
    src/ubuntu/ubuntueditorfactory.h \
    src/ubuntu/ubuntucmakecache.h \
    src/ubuntu/ubuntutestcontrol.h \
    src/ubuntu/ubuntupackageoutputparser.h \
    src/ubuntu/ubuntuprojecthelper.h \
    src/ubuntu/ubuntuscopefinalizer.h \
    src/ubuntu/wizards/ubuntuprojectmigrationwizard.h \
    src/ubuntu/targetupgrademanager.h \
    src/ubuntu/ubuntupackagingmodel.h \
    src/ubuntu/ubuntufixmanifeststep.h \
    src/ubuntu/wizards/ubuntufatpackagingwizard.h \
    src/ubuntu/ubuntulocalruncontrolfactory.h \
    src/ubuntu/ubuntulocalscopedebugsupport.h\
    src/ubuntu/ubuntusettingsprojectdefaultspage.h \
    src/ubuntu/settings.h \
    src/ubuntu/device/container/containerdevice.h \
    src/ubuntu/device/container/containerdevicefactory.h \
    src/ubuntu/device/container/containerprocesslist.h \
    src/ubuntu/device/container/containerdevicesignaloperation.h \
    src/ubuntu/device/container/containerdeviceprocess.h

#remote device support
SOURCES += \
    src/ubuntu/device/remote/abstractremoterunsupport.cpp\
    src/ubuntu/device/remote/ubuntudevicefactory.cpp \
    src/ubuntu/device/remote/ubuntudevice.cpp \
    src/ubuntu/device/remote/ubuntudevicenotifier.cpp \
    src/ubuntu/device/remote/ubuntuemulatornotifier.cpp \
    src/ubuntu/device/remote/ubuntudevicesignaloperation.cpp \
    src/ubuntu/device/remote/ubunturemoterunconfiguration.cpp \
    src/ubuntu/device/remote/ubunturemotedeployconfiguration.cpp \
    src/ubuntu/device/remote/ubunturemoteruncontrolfactory.cpp \
    src/ubuntu/device/remote/ubunturemoteruncontrol.cpp \
    src/ubuntu/device/remote/ubunturemotedebugsupport.cpp \
    src/ubuntu/device/remote/ubunturemoteanalyzesupport.cpp \
    src/ubuntu/device/remote/ubunturemoterunner.cpp \
    src/ubuntu/device/remote/ubuntuwaitfordevicedialog.cpp

HEADERS +=  \
    src/ubuntu/device/remote/abstractremoterunsupport.h \
    src/ubuntu/device/remote/ubuntudevicefactory.h \
    src/ubuntu/device/remote/ubuntudevice.h \
    src/ubuntu/device/remote/ubuntudevicesignaloperation.h \
    src/ubuntu/device/remote/ubuntudevicenotifier.h \
    src/ubuntu/device/remote/ubuntuemulatornotifier.h \
    src/ubuntu/device/remote/ubunturemoterunconfiguration.h \
    src/ubuntu/device/remote/ubunturemotedeployconfiguration.h \
    src/ubuntu/device/remote/ubunturemoteruncontrolfactory.h \
    src/ubuntu/device/remote/ubunturemoteruncontrol.h \
    src/ubuntu/device/remote/ubunturemotedebugsupport.h \
    src/ubuntu/device/remote/ubunturemoteanalyzesupport.h \
    src/ubuntu/device/remote/ubunturemoterunner.h \
    src/ubuntu/device/remote/ubuntuwaitfordevicedialog.h

FORMS += \
    src/ubuntu/device/remote/ubunturemoterunconfigurationwidget.ui \

INCLUDEPATH += $$OUT_PWD \
               $$PWD/src

xml_desc.target=com.ubuntu.sdk.ClickChrootAgent.xml
xml_desc.commands=$$[QT_INSTALL_BINS]/qdbuscpp2xml -o $$xml_desc.target $$PWD/chroot-agent/chrootagent.h
xml_desc.depends=$$PWD/chroot-agent/chrootagent.h
QMAKE_EXTRA_TARGETS+=xml_desc

DBUS_INTERFACES += $$xml_desc.target
