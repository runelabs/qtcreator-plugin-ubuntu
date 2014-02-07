include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(
    %DISPLAYNAME_LOWER%backend_SRCS
    modules/%DISPLAYNAME_LOWER%/backend.cpp
    modules/%DISPLAYNAME_LOWER%/mytype.cpp
)

add_library(%DISPLAYNAME_LOWER%backend MODULE
    ${%DISPLAYNAME_LOWER%backend_SRCS}
)

set_target_properties(%DISPLAYNAME_LOWER%backend PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY %DISPLAYNAME_LOWER%)

qt5_use_modules(%DISPLAYNAME_LOWER%backend Gui Qml Quick)

# Copy qmldir file to build dir for running in QtCreator
add_custom_target(%DISPLAYNAME_LOWER%backend-qmldir ALL
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/modules/%DISPLAYNAME_LOWER%/qmldir ${CMAKE_CURRENT_BINARY_DIR}/%DISPLAYNAME_LOWER%
    DEPENDS ${QMLFILES}
)

# Install plugin file
install(TARGETS %DISPLAYNAME_LOWER%backend DESTINATION ${QT_IMPORTS_DIR}/%DISPLAYNAME_LOWER%/)
install(FILES   modules/%DISPLAYNAME_LOWER%/qmldir DESTINATION ${QT_IMPORTS_DIR}/%DISPLAYNAME_LOWER%/)
