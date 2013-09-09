QT += network qml quick webkitwidgets script scripttools declarative

include(../plugin.pri)

#####################################
## Project files

FORMS += \
    ubuntudeviceswidget.ui \
    ubuntupackagingwidget.ui

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
    ubuntuprocess.cpp

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
    ubuntuprocess.h

