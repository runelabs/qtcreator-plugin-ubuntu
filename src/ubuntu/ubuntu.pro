QT += network qml quick webkitwidgets script scripttools declarative

include(../plugin.pri)

#####################################
# required for Ubuntu Device Notifier
CONFIG += link_pkgconfig

PKGCONFIG += libudev
#####################################

#####################################
## Project files

FORMS += \
    ubuntudeviceswidget.ui \
    ubuntupackagingwidget.ui \
    ubuntusettingswidget.ui \
    ubuntusecuritypolicypickerdialog.ui \
    ubuntusettingsdeviceconnectivitywidget.ui \
    ubuntusettingsclickwidget.ui \
    ubuntuclickdialog.ui \
    ubuntucreatenewchrootdialog.ui

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
    ubuntuprojectapplicationwizard.cpp \
    ubuntumenu.cpp \
    ubuntuprojectmanager.cpp \
    ubuntuproject.cpp \
    ubuntuprojectfile.cpp \
    ubuntuprojectnode.cpp \
    ubunturunconfiguration.cpp \
    ubunturunconfigurationfactory.cpp \
    ubunturuncontrol.cpp \
    ubunturuncontrolfactory.cpp \
    ubuntuprojectapp.cpp \
    ubuntuversion.cpp \
    ubuntufeatureprovider.cpp \
    ubuntuversionmanager.cpp \
    ubuntuircmode.cpp \
    ubuntuapimode.cpp \
    ubuntucoreappsmode.cpp \
    ubuntuwikimode.cpp \
    ubuntupackagingmode.cpp \
    ubuntupackagingwidget.cpp \
    ubuntubzr.cpp \
    ubuntuclickmanifest.cpp \
    ubuntuwebmode.cpp \
    ubuntupastebinmode.cpp \
    ubuntudeviceswidget.cpp \
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
    ubuntuvalidationresultmodel.cpp

HEADERS += \
    ubuntuplugin.h \
    ubuntu_global.h \
    ubuntuconstants.h \
    ubuntuwelcomemode.h \
    ubuntuprojectapplicationwizard.h \
    ubuntumenu.h \
    ubuntushared.h \
    ubuntuprojectmanager.h \
    ubuntuproject.h \
    ubuntuprojectfile.h \
    ubuntuprojectnode.h \
    ubunturunconfiguration.h \
    ubunturunconfigurationfactory.h \
    ubunturuncontrol.h \
    ubunturuncontrolfactory.h \
    ubuntuprojectapp.h \
    ubuntuversion.h \
    ubuntufeatureprovider.h \
    ubuntuversionmanager.h \
    ubuntuircmode.h \
    ubuntuapimode.h \
    ubuntucoreappsmode.h \
    ubuntuwikimode.h \
    ubuntupackagingmode.h \
    ubuntupackagingwidget.h \
    ubuntubzr.h \
    ubuntuclickmanifest.h \
    ubuntuwebmode.h \
    ubuntupastebinmode.h \
    ubuntudevicemode.h \
    ubuntudeviceswidget.h \
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
    ubuntuvalidationresultmodel.h

