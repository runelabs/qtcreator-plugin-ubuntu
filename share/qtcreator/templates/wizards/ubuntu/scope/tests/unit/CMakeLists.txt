
# This command ensures that test-scope.py has execute permission
execute_process(COMMAND chmod +x "${CMAKE_CURRENT_SOURCE_DIR}/test-scope.py")

# Register the test with CTest
add_test(
  scope-unit-tests
@if "%ContentType%" == "empty"
  "${CMAKE_CURRENT_SOURCE_DIR}/test-scope.py" "${SCOPE_NAME}" "${TEST_SCOPE_DIRECTORY}/${SCOPE_NAME}.ini"
@elsif "%ContentType%".substring(0, "network".length) === "network"
  "${CMAKE_CURRENT_SOURCE_DIR}/test-scope.py" "${SCOPE_NAME}" "${TEST_SCOPE_DIRECTORY}/${SCOPE_NAME}.ini" "${FAKE_SERVER}"
@endif
)
