
# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
configure_file(
  "${CMAKE_SOURCE_DIR}/data/%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini"
  "${CMAKE_CURRENT_BINARY_DIR}/%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini"
  @ONLY
)

# Build an object library for the scope code
add_library(
  scope-static OBJECT
  api/client.cpp
  scope/preview.cpp
  scope/query.cpp
  scope/scope.cpp
)

# Ensure we export all the symbols
set_target_properties(
  scope-static
  PROPERTIES
    LINK_FLAGS "-Wl,--export-all-symbols"
)

# Build a shared library containing our scope code.
# This will be the actual plugin that is loaded.
add_library(
  scope SHARED
  $<TARGET_OBJECTS:scope-static>
)

# Link against the object library and our external library dependencies
target_link_libraries(
  scope
  ${SCOPE_LDFLAGS}
  ${Boost_LIBRARIES}
)

# Set the correct library output name to conform to the securiry policy 
set_target_properties(
  scope
  PROPERTIES
    OUTPUT_NAME "%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%"
)

# Install the scope shared library
install(
  TARGETS scope
  LIBRARY DESTINATION ${SCOPE_INSTALL_DIR}
)
