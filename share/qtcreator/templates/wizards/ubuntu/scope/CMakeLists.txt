project(%ProjectName% CXX)
cmake_minimum_required(VERSION 2.8.10)
set(SCOPE_NAME %ProjectName%)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pedantic -Wall -Wextra")
set(UBUNTU_PROJECT_TYPE "Scope" CACHE INTERNAL "Tells QtCreator this is a Scope project")

set(INI_INSTALLDIR "/${SCOPE_NAME}")
set(SCOPE_INSTALLDIR "/${SCOPE_NAME}")

include(FindPkgConfig)
pkg_check_modules(UNITY_SCOPES libunity-scopes>=0.4.0 REQUIRED)

add_subdirectory(src)
add_subdirectory(data)
enable_testing()
add_subdirectory(test)

install(FILES manifest.json DESTINATION "/")
install(FILES "${CMAKE_PROJECT_NAME}.json" DESTINATION "/")
