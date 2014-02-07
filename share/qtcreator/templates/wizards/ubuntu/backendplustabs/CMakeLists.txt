project(%DISPLAYNAME% C CXX)
cmake_minimum_required(VERSION 2.8.9)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt5Core)
find_package(Qt5Qml)
find_package(Qt5Quick)
#find_package(ubuntu-sdk-libs)

set(CMAKE_AUTOMOC ON)

# Standard install paths
include(GNUInstallDirs)

# components PATH
#execute_process(
#    COMMAND qmake -query QT_INSTALL_QML
#    OUTPUT_VARIABLE QT_IMPORTS_DIR
#    OUTPUT_STRIP_TRAILING_WHITESPACE
#)
set(QT_IMPORTS_DIR "/usr/lib/arm-linux-gnueabihf/qt5/qml")

option(INSTALL_TESTS "Install the tests on make install" on)
option(CLICK_MODE "Installs to a contained location" off)

set(APP_NAME          %DISPLAYNAME%)
set(%DISPLAYNAME_UPPER%_DIR ${CMAKE_INSTALL_DATADIR}/qml/%DISPLAYNAME%)
set(MAIN_QML          "%DISPLAYNAME%.qml")
set(ICON              "grapics/toolbarIcon@8.png")
include(Click)


add_subdirectory(app)
add_subdirectory(backend)

 add_custom_target("autopilot" chmod +x ${CMAKE_SOURCE_DIR}/app/tests/autopilot/run
                     COMMAND ${CMAKE_SOURCE_DIR}/app/tests/autopilot/run
                     DEPENDS %DISPLAYNAME%backend %DISPLAYNAME%backend-qmldir
                     WORKING_DIRECTORY ./app)

 add_custom_target("check" /usr/bin/qmltestrunner -input ${CMAKE_SOURCE_DIR}/app/tests/unit -import ${CMAKE_BINARY_DIR}/backend
                     DEPENDS %DISPLAYNAME%backend %DISPLAYNAME%backend-qmldir
                     WORKING_DIRECTORY ./app)

 add_custom_target("run" /usr/bin/qmlscene -I ${CMAKE_BINARY_DIR}/backend  ${CMAKE_SOURCE_DIR}/app/%DISPLAYNAME%.qml
                     DEPENDS %DISPLAYNAME%backend %DISPLAYNAME%backend-qmldir
                     WORKING_DIRECTORY ./app)
