
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
  ${TEST_LDFLAGS}
  ${Boost_LIBRARIES}
)

add_test(
  scope-unit-tests
  scope-unit-tests
)
