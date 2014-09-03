
add_definitions(-DPACKAGE_NAME="%ClickDomain:l%.%ProjectName:l%")
add_definitions(-DSCOPE_NAME="%ClickDomain:l%.%ProjectName:l%_%ClickHookName:l%")

add_executable(
  scope-unit-tests
  scope/test-scope.cpp
  $<TARGET_OBJECTS:scope-static>
)

target_link_libraries(
  scope-unit-tests
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
  ${SCOPE_LDFLAGS}
  ${Boost_LIBRARIES}
)

add_test(
  scope-unit-tests
  scope-unit-tests
)
