add_library(
  ${SCOPE_LIB_TARGET_NAME} SHARED
  %ClickHookName:l%-preview.cpp
  %ClickHookName:l%-query.cpp
  %ClickHookName:l%-scope.cpp
)
target_link_libraries(${SCOPE_LIB_TARGET_NAME} ${UNITY_SCOPES_LDFLAGS})
set_property(TARGET ${SCOPE_LIB_TARGET_NAME} PROPERTY COMPILE_FLAGS ${UNITY_SCOPES_CFLAGS})

install(TARGETS ${SCOPE_LIB_TARGET_NAME}
LIBRARY DESTINATION "${SCOPE_INSTALLDIR}"
)
