# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
configure_file(%DISPLAYNAME%-scope.ini.in ../src/%DISPLAYNAME%-scope.ini)

INSTALL(FILES ${CMAKE_BINARY_DIR}/src/%DISPLAYNAME%-scope.ini
DESTINATION ${INI_INSTALLDIR})
