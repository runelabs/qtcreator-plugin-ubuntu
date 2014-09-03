
# Install the scope ini file
install(
  FILES "%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%.ini"
  DESTINATION ${SCOPE_INSTALL_DIR}
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
