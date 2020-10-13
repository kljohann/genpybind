find_package(GTest 1.10 REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 17)

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

find_program(LIT_COMMAND NAMES lit.py lit)
find_program(FILECHECK_COMMAND NAMES FileCheck)
if(FILECHECK_COMMAND STREQUAL "FILECHECK_COMMAND-NOTFOUND")
  message(WARNING "FileCheck not found, skipping (some) tests")
elseif(LIT_COMMAND STREQUAL "LIT_COMMAND-NOTFOUND")
  message(WARNING "lit.py not found, skipping (some) tests")
else()
  add_custom_target(lit-tests-run
    COMMAND
    ${LIT_COMMAND} -v
    --param genpybind-tool=$<TARGET_FILE:genpybind-tool>
    --param FileCheck=${FILECHECK_COMMAND}
    ${CMAKE_CURRENT_SOURCE_DIR}/lit
    COMMENT "Running lit tests"
    VERBATIM USES_TERMINAL
  )
  add_dependencies(test lit-tests-run)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Each `TEST_NAME.cpp` file in `integration/` defines an integration test, for
# which a shared library target called `integration-TEST_NAME` is created.
# The corresponding header file is analyzed using genpybind in order to generate
# code for a pybind11-based Python module `TEST_NAME`.
# Finally `TEST_NAME_test.py` is executed using pytest.
file(GLOB integration_tests CONFIGURE_DEPENDS "integration/*.cpp")
foreach(test_implementation IN LISTS integration_tests)
  get_filename_component(test_name ${test_implementation} NAME_WE)
  set(test_target integration-${test_name})
  get_filename_component(test_header integration/${test_name}.h ABSOLUTE)
  set(test_bindings ${CMAKE_CURRENT_BINARY_DIR}/genpybind-${test_name}.cpp)
  string(MAKE_C_IDENTIFIER ${test_name} module_name)

  add_library(${test_target} SHARED ${test_implementation})
  target_include_directories(${test_target}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<TARGET_PROPERTY:pybind11::module,INTERFACE_INCLUDE_DIRECTORIES>
  )
  add_custom_command(
    OUTPUT ${test_bindings}
    MAIN_DEPENDENCY ${test_header}
    DEPENDS genpybind-tool
    IMPLICIT_DEPENDS CXX ${test_header}
    COMMAND $<TARGET_FILE:genpybind-tool>
    ARGS -p ${CMAKE_BINARY_DIR} ${test_header} --extra-arg=-D__GENPYBIND__
    > ${test_bindings}
    COMMENT "Analyzing ${test_header}"
    VERBATIM USES_TERMINAL
  )
  pybind11_add_module(${module_name} MODULE ${test_bindings})
  target_link_libraries(${module_name} PUBLIC ${test_target} genpybind-runtime)
  add_custom_target(${test_target}-test
    COMMAND Python3::Interpreter -m pytest -q
    ${CMAKE_CURRENT_SOURCE_DIR}/integration/${test_name}_test.py
    DEPENDS ${module_name}
    COMMENT "Running ${test_name}_test.py"
    VERBATIM USES_TERMINAL
  )
  add_dependencies(test ${test_target}-test)
  set_target_properties(${test_target} ${module_name}
    PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "")
endforeach()
