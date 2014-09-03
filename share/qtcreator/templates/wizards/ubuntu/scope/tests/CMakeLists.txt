
include(FindGMock)

pkg_check_modules(
  TEST
  process-cpp
  REQUIRED
)

include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${GMOCK_INCLUDE_DIRS}
  ${TEST_INCLUDE_DIRS}
)

add_definitions(
  -DFAKE_SERVER="${CMAKE_CURRENT_SOURCE_DIR}/server/server.py"
)

add_subdirectory(unit)
