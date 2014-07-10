include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(
    %ClickHookName:s%backend_SRCS
    modules/%ClickHookName:s%/backend.cpp
    modules/%ClickHookName:s%/mytype.cpp
)

add_library(%ClickHookName:s%backend MODULE
    ${%ClickHookName:s%backend_SRCS}
)

set_target_properties(%ClickHookName:s%backend PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY %ClickHookName:s%)

qt5_use_modules(%ClickHookName:s%backend Gui Qml Quick)

# Copy qmldir file to build dir for running in QtCreator
add_custom_target(%ClickHookName:s%backend-qmldir ALL
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/modules/%ClickHookName:s%/qmldir ${CMAKE_CURRENT_BINARY_DIR}/%ClickHookName:s%
    DEPENDS ${QMLFILES}
)

# Install plugin file
install(TARGETS %ClickHookName:s%backend DESTINATION ${QT_IMPORTS_DIR}/%ClickHookName:s%/)
install(FILES   modules/%ClickHookName:s%/qmldir DESTINATION ${QT_IMPORTS_DIR}/%ClickHookName:s%/)
