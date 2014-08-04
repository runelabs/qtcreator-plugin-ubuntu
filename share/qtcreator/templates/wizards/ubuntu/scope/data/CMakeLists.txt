# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
configure_file(
  "%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini.in"
  "${CMAKE_BINARY_DIR}/src/%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini"
)

INSTALL(
  FILES "${CMAKE_BINARY_DIR}/src/%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini"
  DESTINATION "${INI_INSTALLDIR}"
)
