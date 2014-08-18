project(%ProjectName:l% CXX)
cmake_minimum_required(VERSION 2.8.10)
set(SCOPE_NAME %ClickHookName:l%)
set(SCOPE_LIB_TARGET_NAME %ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pedantic -Wall -Wextra")
set(UBUNTU_PROJECT_TYPE "Scope" CACHE INTERNAL "Tells QtCreator this is a Scope project")

set(INI_INSTALLDIR "/${SCOPE_NAME}")
set(SCOPE_INSTALLDIR "/${SCOPE_NAME}")

include(FindPkgConfig)
pkg_check_modules(UNITY_SCOPES libunity-scopes>=0.5.0 REQUIRED)

add_subdirectory(src)
add_subdirectory(data)
enable_testing()
add_subdirectory(test)

install(FILES manifest.json DESTINATION "/")
install(FILES "%ClickHookName:l%.apparmor" DESTINATION "/")

add_custom_target("%ProjectName:l%_ClickFiles" ALL SOURCES "%ClickHookName:l%.apparmor" "manifest.json")
