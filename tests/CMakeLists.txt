# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

find_package(GTest 1.10 REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 17)

add_executable(genpybind-tests)
llvm_update_compile_flags(genpybind-tests)
target_link_libraries(genpybind-tests
  PRIVATE genpybind GTest::gmock GTest::gmock_main)
file(GLOB_RECURSE test_files CONFIGURE_DEPENDS "*_test.cpp")
target_sources(genpybind-tests PRIVATE ${test_files})

add_custom_target(genpybind-tests-run
  COMMAND $<TARGET_FILE:genpybind-tests>
  COMMENT "Running unit tests"
  VERBATIM
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
    VERBATIM
  )
  add_dependencies(test lit-tests-run)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# genpybind_add_module(<target-name>
#                      HEADER <header-file>
#                      [LINK_LIBRARIES <targets>...]
#                      [NUM_BINDING_FILES <count>]
#                      <pybind11_add_module-args>...)
# Creates a pybind11 module target based on auto-generated bindings for
# the given header file.  If specified, the generated code is split into
# several intermediate files to take advantage of parallel builds.
function(genpybind_add_module target_name)
  set(flag_opts "")
  set(value_opts HEADER)
  set(multi_opts LINK_LIBRARIES NUM_BINDING_FILES)
  cmake_parse_arguments(
    ARG "${flag_opts}" "${value_opts}" "${multi_opts}" ${ARGN}
  )

  if(NOT DEFINED ARG_NUM_BINDING_FILES OR ARG_NUM_BINDING_FILES LESS 1)
    set(ARG_NUM_BINDING_FILES 1)
  endif()
  math(EXPR index_range "${ARG_NUM_BINDING_FILES} - 1")

  set(bindings "")
  foreach(idx RANGE ${index_range})
    list(
      APPEND bindings
      "${CMAKE_CURRENT_BINARY_DIR}/genpybind-${target_name}-${idx}.cpp"
    )
  endforeach()

  list(TRANSFORM bindings PREPEND "-o=" OUTPUT_VARIABLE output_args)
  add_custom_command(
    OUTPUT ${bindings}
    MAIN_DEPENDENCY ${ARG_HEADER}
    DEPENDS genpybind-tool
    IMPLICIT_DEPENDS CXX ${ARG_HEADER}
    COMMAND $<TARGET_FILE:genpybind-tool>
    ARGS -p ${CMAKE_BINARY_DIR} ${ARG_HEADER}
    ${output_args}
    COMMENT "Analyzing ${ARG_HEADER}"
    VERBATIM
  )

  pybind11_add_module(${target_name} ${bindings} ${ARG_UNPARSED_ARGUMENTS})
  target_link_libraries(
    ${target_name} PUBLIC ${ARG_LINK_LIBRARIES} genpybind-runtime
  )
endfunction()

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
  string(MAKE_C_IDENTIFIER ${test_name} module_name)

  add_library(${test_target} SHARED ${test_implementation})
  # -Wno-comment: for multi-line comments used in lit tests
  target_compile_options(${test_target} PUBLIC -Wno-comment)
  target_include_directories(${test_target}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<TARGET_PROPERTY:pybind11::module,INTERFACE_INCLUDE_DIRECTORIES>
  )
  set(num_files 1)
  if(module_name STREQUAL "operators")
    set(num_files 8)
  endif()
  genpybind_add_module(
    ${module_name} MODULE
    LINK_LIBRARIES ${test_target}
    NUM_BINDING_FILES ${num_files}
    HEADER ${test_header}
  )
  add_custom_target(${test_target}-test
    COMMAND Python3::Interpreter -m pytest -q
    ${CMAKE_CURRENT_SOURCE_DIR}/integration/${test_name}_test.py
    DEPENDS ${module_name}
    COMMENT "Running ${test_name}_test.py"
    VERBATIM
  )
  add_dependencies(test ${test_target}-test)
  set_target_properties(${test_target} ${module_name}
    PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "")
endforeach()

target_link_libraries(integration-typedefs-across-modules
  PUBLIC integration-typedefs-across-modules-definition)
add_dependencies(typedefs_across_modules typedefs_across_modules_definition)
