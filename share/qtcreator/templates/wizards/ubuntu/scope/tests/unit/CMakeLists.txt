
# Register the test with CTest
add_test(
  scope-unit-tests
  "${CMAKE_CURRENT_SOURCE_DIR}/scope/test-scope.py" "${SCOPE_NAME}" "${TEST_SCOPE_DIRECTORY}/${SCOPE_NAME}.ini" "${FAKE_SERVER}"
)
