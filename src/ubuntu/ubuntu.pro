QT += network qml quick webkitwidgets script scripttools dbus

include(../plugin.pri)

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
    ubuntupackagingwidget.ui \
    ubuntusettingswidget.ui \
    ubuntusecuritypolicypickerdialog.ui \
    ubuntusettingsdeviceconnectivitywidget.ui \
    ubuntusettingsclickwidget.ui \
    ubuntuclickdialog.ui \
    ubuntucreatenewchrootdialog.ui \
    ubuntupackagestepconfigwidget.ui \
    ubuntumanifesteditor.ui \
    ubuntuapparmoreditor.ui \
    ubunturemoterunconfigurationwidget.ui \
    targetupgrademanagerdialog.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    UbuntuProject.mimetypes.xml \
    manifest.json.template \
    myapp.json.template \
    manifestlib.js

SOURCES += \
    ubuntuplugin.cpp \
    ubuntuwelcomemode.cpp \
    wizards/ubuntuprojectapplicationwizard.cpp \
    ubuntumenu.cpp \
    ubuntuprojectmanager.cpp \
    ubuntuproject.cpp \
    ubuntuprojectfile.cpp \
    ubuntuprojectnode.cpp \
    ubuntuversion.cpp \
    ubuntufeatureprovider.cpp \
    ubuntuversionmanager.cpp \
    ubuntuircmode.cpp \
    ubuntuapimode.cpp \
    ubuntucoreappsmode.cpp \
    ubuntuwikimode.cpp \
    ubuntupackagingmode.cpp \
    ubuntubzr.cpp \
    ubuntuclickmanifest.cpp \
    ubuntuwebmode.cpp \
    ubuntupastebinmode.cpp \
    ubuntudevicemode.cpp \
    ubuntuprocess.cpp \
    ubuntudevicenotifier.cpp \
    ubuntusettingspage.cpp \
    ubuntusettingswidget.cpp \
    ubuntusecuritypolicypickerdialog.cpp \
    ubuntupolicygroupmodel.cpp \
    ubuntupolicygroupinfo.cpp \
    ubuntusettingsdeviceconnectivitypage.cpp \
    ubuntusettingsdeviceconnectivitywidget.cpp \
    ubuntusettingsclickpage.cpp \
    ubuntusettingsclickwidget.cpp \
    ubuntuclicktool.cpp \
    ubuntucreatenewchrootdialog.cpp \
    ubuntuclickdialog.cpp \
    ubuntuvalidationresultmodel.cpp \
    clicktoolchain.cpp \
    ubuntukitmanager.cpp \
    ubuntucmaketool.cpp \
    ubuntucmakebuildconfiguration.cpp \
    ubuntudevicefactory.cpp \
    ubuntudevice.cpp \
    ubunturemoterunconfiguration.cpp \
    ubuntucmakemakestep.cpp \
    ubuntuemulatornotifier.cpp \
    localportsmanager.cpp \
    ubuntulocaldeployconfiguration.cpp \
    ubunturemotedeployconfiguration.cpp \
    ubuntulocalrunconfigurationfactory.cpp \
    ubunturemoteruncontrolfactory.cpp \
    ubuntulocalrunconfiguration.cpp \
    ubuntudevicesmodel.cpp \
    ubunturemoteruncontrol.cpp \
    ubunturemotedebugsupport.cpp \
    ubunturemoteanalyzesupport.cpp \
    ubuntudevicesignaloperation.cpp \
    ubunturemoterunner.cpp \
    abstractremoterunsupport.cpp\
    ubuntushared.cpp \
    ubuntupackagestep.cpp \
    ubuntuqtversion.cpp \
    ubuntudeploystepfactory.cpp \
    ubuntudirectuploadstep.cpp \
    ubuntuhtmlbuildconfiguration.cpp \
    ubuntuqmlbuildconfiguration.cpp \
    wizards/ubuntufirstrunwizard.cpp \
    ubuntuwaitfordevicedialog.cpp \
    ubuntumanifesteditor.cpp \
    ubuntumanifesteditorwidget.cpp \
    ubuntuabstractguieditorwidget.cpp \
    ubuntuabstractguieditor.cpp \
    ubuntuabstractguieditordocument.cpp \
    ubuntuapparmoreditor.cpp \
    ubuntueditorfactory.cpp \
    ubuntucmakecache.cpp \
    ubuntutestcontrol.cpp \
    ubuntupackageoutputparser.cpp \
    ubuntuprojecthelper.cpp \
    wizards/ubuntuprojectmigrationwizard.cpp \
    targetupgrademanager.cpp \
    ubuntupackagingmodel.cpp \
    ubuntufixmanifeststep.cpp \
    wizards/ubuntufatpackagingwizard.cpp

HEADERS += \
    ubuntuplugin.h \
    ubuntu_global.h \
    ubuntuconstants.h \
    ubuntuwelcomemode.h \
    wizards/ubuntuprojectapplicationwizard.h \
    ubuntumenu.h \
    ubuntushared.h \
    ubuntuprojectmanager.h \
    ubuntuproject.h \
    ubuntuprojectfile.h \
    ubuntuprojectnode.h \
    ubuntuversion.h \
    ubuntufeatureprovider.h \
    ubuntuversionmanager.h \
    ubuntuircmode.h \
    ubuntuapimode.h \
    ubuntucoreappsmode.h \
    ubuntuwikimode.h \
    ubuntupackagingmode.h \
    ubuntubzr.h \
    ubuntuclickmanifest.h \
    ubuntuwebmode.h \
    ubuntupastebinmode.h \
    ubuntudevicemode.h \
    ubuntuprocess.h \
    ubuntudevicenotifier.h \
    ubuntusettingspage.h \
    ubuntusettingswidget.h \
    ubuntusecuritypolicypickerdialog.h \
    ubuntupolicygroupmodel.h \
    ubuntupolicygroupinfo.h \
    ubuntusettingsdeviceconnectivitypage.h \
    ubuntusettingsdeviceconnectivitywidget.h \
    ubuntusettingsclickpage.h \
    ubuntusettingsclickwidget.h \
    ubuntuclicktool.h \
    ubuntucreatenewchrootdialog.h \
    ubuntuclickdialog.h \
    ubuntuvalidationresultmodel.h \
    clicktoolchain.h \
    ubuntukitmanager.h \
    ubuntucmaketool.h \
    ubuntucmakebuildconfiguration.h \
    ubuntudevicefactory.h \
    ubuntudevice.h \
    ubunturemoterunconfiguration.h \
    ubuntucmakemakestep.h \
    ubuntuemulatornotifier.h \
    localportsmanager.h \
    ubuntulocaldeployconfiguration.h \
    ubunturemotedeployconfiguration.h \
    ubuntulocalrunconfigurationfactory.h \
    ubunturemoteruncontrolfactory.h \
    ubuntulocalrunconfiguration.h \
    ubuntudevicesmodel.h \
    ubunturemoteruncontrol.h \
    ubunturemotedebugsupport.h \
    ubunturemoteanalyzesupport.h \
    ubuntudevicesignaloperation.h \
    ubunturemoterunner.h \
    abstractremoterunsupport.h \
    ubuntupackagestep.h \
    ubuntuqtversion.h \
    ubuntudeploystepfactory.h \
    ubuntudirectuploadstep.h \
    ubuntuhtmlbuildconfiguration.h \
    ubuntuqmlbuildconfiguration.h \
    wizards/ubuntufirstrunwizard.h \
    ubuntuwaitfordevicedialog.h \
    ubuntumanifesteditor.h \
    ubuntumanifesteditorwidget.h \
    ubuntuabstractguieditorwidget.h \
    ubuntuabstractguieditor.h \
    ubuntuabstractguieditordocument.h \
    ubuntuapparmoreditor.h \
    ubuntueditorfactory.h \
    ubuntucmakecache.h \
    ubuntutestcontrol.h \
    ubuntupackageoutputparser.h \
    ubuntuprojecthelper.h \
    ubuntuscopefinalizer.h \
    wizards/ubuntuprojectmigrationwizard.h \
    targetupgrademanager.h \
    ubuntupackagingmodel.h \
    ubuntufixmanifeststep.h \
    wizards/ubuntufatpackagingwizard.h

INCLUDEPATH+=$$OUT_PWD

xml_desc.target=com.ubuntu.sdk.ClickChrootAgent.xml
xml_desc.commands=qdbuscpp2xml -o $$xml_desc.target $$PWD/../../chroot-agent/chrootagent.h
xml_desc.depends=$$PWD/../../chroot-agent/chrootagent.h
QMAKE_EXTRA_TARGETS+=xml_desc

DBUS_INTERFACES += $$xml_desc.target
