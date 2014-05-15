# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
configure_file(
  "%DISPLAYNAME%.ini.in"
  "${CMAKE_BINARY_DIR}/src/%DISPLAYNAME%.ini"
)

INSTALL(
  FILES "${CMAKE_BINARY_DIR}/src/%DISPLAYNAME%.ini"
  DESTINATION "${INI_INSTALLDIR}"
)
