
# Our test executable.
# It includes the object code from the scope
add_executable(
  scope-unit-tests
  scope/test-scope.cpp
  $<TARGET_OBJECTS:scope-static>
)

# Link against the scope, and all of our test lib dependencies
target_link_libraries(
  scope-unit-tests
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
  ${SCOPE_LDFLAGS}
  ${TEST_LDFLAGS}
  ${Boost_LIBRARIES}
)

@if "%ContentType%".substring(0, "network-netcpp-q".length) === "network-netcpp-q"
qt5_use_modules(
  scope-unit-tests
  Core
)
@endif

# Register the test with CTest
add_test(
  scope-unit-tests
  scope-unit-tests
)
