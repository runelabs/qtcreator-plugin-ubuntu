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

add_definitions(
  -fno-permissive
  -std=c++11
  -pedantic
  -Wall
  -Wextra
  -fPIC
)

include(GNUInstallDirs)
find_package(PkgConfig)

find_package(
  Boost
  REQUIRED
)

pkg_check_modules(
  SCOPE
  libunity-scopes>=0.6.0
  jsoncpp
  net-cpp>=1.1.0
  process-cpp
  REQUIRED
)

include_directories(
  "${CMAKE_SOURCE_DIR}/include"
  ${Boost_INCLUDE_DIRS}
  ${SCOPE_INCLUDE_DIRS}
)

set(UBUNTU_PROJECT_TYPE "Scope" CACHE INTERNAL "Tells QtCreator this is a Scope project")
set(SCOPE_INSTALL_DIR "/%ClickHookName:l%")
add_definitions(-DPACKAGE_NAME="%ClickDomain:l%.%ProjectName:l%")
add_definitions(-DSCOPE_NAME="%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%")

install(FILES manifest.json DESTINATION "/")
install(FILES "%ClickHookName:l%.apparmor" DESTINATION "/")

add_subdirectory(src)
add_subdirectory(data)

enable_testing()
add_subdirectory(tests)
add_custom_target(
  check
  ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure
)

