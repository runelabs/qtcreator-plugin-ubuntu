include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(
    %DISPLAYNAME%backend_SRCS
    modules/%DISPLAYNAME%/backend.cpp
    modules/%DISPLAYNAME%/mytype.cpp
)

add_library(%DISPLAYNAME%backend MODULE
    ${%DISPLAYNAME%backend_SRCS}
)

set_target_properties(%DISPLAYNAME%backend PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY %DISPLAYNAME%)

qt5_use_modules(%DISPLAYNAME%backend Gui Qml Quick)

# Copy qmldir file to build dir for running in QtCreator
add_custom_target(%DISPLAYNAME%backend-qmldir ALL
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/modules/%DISPLAYNAME%/qmldir ${CMAKE_CURRENT_BINARY_DIR}/%DISPLAYNAME%
    DEPENDS ${QMLFILES}
)

# Install plugin file
install(TARGETS %DISPLAYNAME%backend DESTINATION ${QT_IMPORTS_DIR}/%DISPLAYNAME%/)
install(FILES   modules/%DISPLAYNAME%/qmldir DESTINATION ${QT_IMPORTS_DIR}/%DISPLAYNAME%/)
