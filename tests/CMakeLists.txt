find_package(GTest 1.10 REQUIRED CONFIG)

add_executable(genpybind-tests)
target_link_libraries(genpybind-tests
  PRIVATE genpybind GTest::gmock GTest::gmock_main)
file(GLOB_RECURSE test_files CONFIGURE_DEPENDS "*_test.cpp")
target_sources(genpybind-tests PRIVATE ${test_files})

add_custom_target(genpybind-tests-run
  COMMAND $<TARGET_FILE:genpybind-tests>
  COMMENT "Running unit tests"
  VERBATIM USES_TERMINAL
)
add_dependencies(test genpybind-tests-run)
