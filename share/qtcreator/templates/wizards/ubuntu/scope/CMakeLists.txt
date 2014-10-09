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

include(GNUInstallDirs)
find_package(PkgConfig)

# We depend on Boost for string trimming
find_package(
  Boost
  REQUIRED
)

# Search for our dependencies
pkg_check_modules(
  SCOPE
  libunity-scopes>=0.6.0
@if "%ContentType%" == "network"
  jsoncpp
  net-cpp>=1.1.0
@endif
  REQUIRED
)

# Add our dependencies to the include paths
include_directories(
  "${CMAKE_SOURCE_DIR}/include"
  ${Boost_INCLUDE_DIRS}
  ${SCOPE_INCLUDE_DIRS}
)

set(UBUNTU_PROJECT_TYPE "Scope" CACHE INTERNAL "Tells QtCreator this is a Scope project")
set(SCOPE_INSTALL_DIR "/%ClickHookName:l%")

# If we need to refer to the scope's name or package in code, these definitions will help
add_definitions(-DPACKAGE_NAME="%ClickDomain:l%.%ProjectName:l%")
add_definitions(-DSCOPE_NAME="%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%")

#This command figures out the target architecture and puts it into the manifest file
execute_process(
  COMMAND dpkg-architecture -qDEB_HOST_ARCH
  OUTPUT_VARIABLE CLICK_ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(manifest.json.in ${CMAKE_CURRENT_BINARY_DIR}/manifest.json)

# Install the click manifest
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/manifest.json DESTINATION "/")
install(FILES "%ClickHookName:l%.apparmor" DESTINATION "/")

# Make this file show up in QtCreator
add_custom_target(hidden_files
  ALL
  SOURCES
    manifest.json.in
    "%ClickHookName:l%.apparmor"
)

# Add our main directories
add_subdirectory(src)
add_subdirectory(data)

# Set up the tests
enable_testing()
add_subdirectory(tests)
add_custom_target(
  check
  ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure
)
