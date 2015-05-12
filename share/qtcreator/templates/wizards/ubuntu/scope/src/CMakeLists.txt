
# Put the ini files in the build directory next to the scope
# .so file so that the test tools can find them easily.
intltool_merge_translations(
  "scope.ini.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}.ini"
  ALL
  UTF8
)
intltool_merge_translations(
  "scope-settings.ini.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}-settings.ini"
  ALL
  UTF8
)

# Install the scope ini files
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}.ini"
  DESTINATION ${SCOPE_INSTALL_DIR}
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${SCOPE_NAME}-settings.ini"
  DESTINATION ${SCOPE_INSTALL_DIR}
)

# Put the logo file in the build directory next to the scope
# .ini file so that the test tools can find it easily.
configure_file(
  "logo.png"
  "${CMAKE_CURRENT_BINARY_DIR}/logo.png"
  @ONLY
  COPYONLY
)

# Install the scope images
install(
  FILES
    "icon.png"
    "logo.png"
    "screenshot.png"
  DESTINATION
    "${SCOPE_INSTALL_DIR}"
)

# Find all the sources and headers
file(GLOB_RECURSE
  SCOPE_SOURCES
  "*.cpp"
)
file(GLOB_RECURSE
  SCOPE_HEADERS
  "*.h"
)

# Build an object library for the scope code
add_library(
  scope-static OBJECT
  ${SCOPE_SOURCES}
  ${SCOPE_HEADERS}
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

@if "%ContentType%".substring(0, "network-netcpp-q".length) === "network-netcpp-q"
qt5_use_modules(
  scope
  Core
)
@endif

# Set the correct library output name to conform to the securiry policy 
set_target_properties(
  scope
  PROPERTIES
    OUTPUT_NAME "${SCOPE_NAME}"
)

# Install the scope shared library
install(
  TARGETS scope
  LIBRARY DESTINATION ${SCOPE_INSTALL_DIR}
)
