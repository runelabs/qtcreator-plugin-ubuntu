project(%ProjectName:l% CXX)
cmake_minimum_required(VERSION 2.8.10)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

# We require g++ 4.9, to avoid ABI breakage with earlier version.
set(cxx_version_required 4.9)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (NOT CMAKE_CXX_COMPILER_VERSION MATCHES "^${cxx_version_required}")
        message(FATAL_ERROR "g++ version must be ${cxx_version_required}!")
    endif()
endif()

# Set strict and naggy C++ compiler flags, and enable C++11
add_definitions(
  -fno-permissive
  -std=c++11
  -pedantic
  -Wall
  -Wextra
  -fPIC
  -DQT_NO_KEYWORDS
)

# Search for our dependencies
include(GNUInstallDirs)
find_package(PkgConfig)
find_package(Intltool)
@if "%ContentType%".substring(0, "network-netcpp-q".length) === "network-netcpp-q"
find_package(Qt5Core REQUIRED)
@endif

pkg_check_modules(
  SCOPE
  libunity-scopes>=0.6.0
@if "%ContentType%" == "network-netcpp-json"
  jsoncpp
@endif
@if "%ContentType%".substring(0, "network-netcpp".length) === "network-netcpp"
  net-cpp>=1.1.0
@endif
  REQUIRED
)

# Add our dependencies to the include paths
include_directories(
  "${CMAKE_SOURCE_DIR}/src"
  ${SCOPE_INCLUDE_DIRS}
@if "%ContentType%".substring(0, "network-netcpp-q".length) === "network-netcpp-q"
  ${Qt5Core_INCLUDE_DIRS}
@endif
)

# Do not remove this line, its required for the correct functionality of the Ubuntu-SDK
set(UBUNTU_MANIFEST_PATH "manifest.json.in" CACHE INTERNAL "Tells QtCreator location and name of the manifest file")
set(UBUNTU_PROJECT_TYPE "Scope" CACHE INTERNAL "Tells QtCreator this is a Scope project")

# Important project paths
set(CMAKE_INSTALL_PREFIX /)
set(SCOPE_INSTALL_DIR "/%ClickHookName:l%")
set(GETTEXT_PACKAGE "%ProjectName:l%")
set(PACKAGE_NAME "%ProjectName:l%.%ClickDomain:l%")
set(SCOPE_NAME "${PACKAGE_NAME}_%ClickHookName:l%")

# If we need to refer to the scope's name or package in code, these definitions will help
add_definitions(-DPACKAGE_NAME="${PACKAGE_NAME}")
add_definitions(-DSCOPE_NAME="${SCOPE_NAME}")
add_definitions(-DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}")

# This command figures out the target architecture and puts it into the manifest file
execute_process(
  COMMAND dpkg-architecture -qDEB_HOST_ARCH
  OUTPUT_VARIABLE CLICK_ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Configure and install the click manifest and apparmor files
configure_file(manifest.json.in ${CMAKE_CURRENT_BINARY_DIR}/manifest.json)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/manifest.json DESTINATION "/")
install(FILES "%ClickHookName:l%.apparmor" DESTINATION "/")

# Make these files show up in QtCreator
file(GLOB_RECURSE
  _PO_FILES
  "po/*.po"
)
add_custom_target(hidden_files
  ALL
  SOURCES
    manifest.json.in
    %ClickHookName:l%.apparmor
    src/data/%ClickHookName:l%.ini.in
    src/data/%ClickHookName:l%-settings.ini.in
    po/POTFILES.in
    po/${GETTEXT_PACKAGE}.pot
    ${_PO_FILES}
)

# Add our main directories
add_subdirectory(po)
add_subdirectory(src)

# Set up the tests
enable_testing()
add_subdirectory(tests)
add_custom_target(
  check
  ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure
)
