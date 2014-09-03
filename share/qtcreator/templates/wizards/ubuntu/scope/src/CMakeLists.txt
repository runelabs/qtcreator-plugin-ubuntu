
# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
configure_file(
  "${CMAKE_SOURCE_DIR}/data/%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini"
  "${CMAKE_CURRENT_BINARY_DIR}/%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini"
  @ONLY
)

add_definitions(-DPACKAGE_NAME="%ClickDomain:l%.%ProjectName:l%")
add_definitions(-DSCOPE_NAME="%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%")

add_library(
  scope-static OBJECT
  api/client.cpp
  scope/preview.cpp
  scope/query.cpp
  scope/scope.cpp
)

set_target_properties(
  scope-static
  PROPERTIES
    LINK_FLAGS "-Wl,--export-all-symbols"
)

add_library(
  scope SHARED
  $<TARGET_OBJECTS:scope-static>
)

target_link_libraries(
  scope
  ${SCOPE_LDFLAGS}
  ${Boost_LIBRARIES}
)

set_target_properties(
  scope
  PROPERTIES
    OUTPUT_NAME "%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%"
)

install(
  TARGETS scope
  LIBRARY DESTINATION ${SCOPE_INSTALL_DIR}
)
