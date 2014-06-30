include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(
    %ProjectName%backend_SRCS
    modules/%ProjectName%/backend.cpp
    modules/%ProjectName%/mytype.cpp
)

add_library(%ProjectName%backend MODULE
    ${%ProjectName%backend_SRCS}
)

set_target_properties(%ProjectName%backend PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY %ProjectName%)

qt5_use_modules(%ProjectName%backend Gui Qml Quick)

# Copy qmldir file to build dir for running in QtCreator
add_custom_target(%ProjectName%backend-qmldir ALL
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/modules/%ProjectName%/qmldir ${CMAKE_CURRENT_BINARY_DIR}/%ProjectName%
    DEPENDS ${QMLFILES}
)

# Install plugin file
install(TARGETS %ProjectName%backend DESTINATION ${QT_IMPORTS_DIR}/%ProjectName%/)
install(FILES   modules/%ProjectName%/qmldir DESTINATION ${QT_IMPORTS_DIR}/%ProjectName%/)
