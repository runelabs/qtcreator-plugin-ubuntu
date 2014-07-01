# Put the ini file in the build directory next to the scope
# .so file so test tools can find both easily.
configure_file(
  "%ClickDomain%.%ProjectName%_%ClickHookName%.ini.in"
  "${CMAKE_BINARY_DIR}/src/%ClickDomain%.%ProjectName%_%ClickHookName%.ini"
)

INSTALL(
  FILES "${CMAKE_BINARY_DIR}/src/%ClickDomain%.%ProjectName%_%ClickHookName%.ini"
  DESTINATION "${INI_INSTALLDIR}"
)
