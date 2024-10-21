# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

# Each `TEST_NAME.cpp` file in defines an integration test, for which a shared
# library target called `integration-TEST_NAME` is created.
# The corresponding header file is analyzed using genpybind in order to generate
# code for a pybind11-based Python module `TEST_NAME`.
# Finally, `TEST_NAME_test.py` is executed using pytest.
file(GLOB integration_tests CONFIGURE_DEPENDS "*.cpp")
foreach(test_implementation IN LISTS integration_tests)
  get_filename_component(test_name ${test_implementation} NAME_WE)
  set(test_target integration-${test_name})
  get_filename_component(test_header ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.h ABSOLUTE)
  string(MAKE_C_IDENTIFIER ${test_name} module_name)

  add_library(${test_target} SHARED ${test_implementation})
  # -Wno-comment: for multi-line comments used in lit tests
  target_compile_options(${test_target} PUBLIC -Wno-comment)
  target_include_directories(${test_target}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<TARGET_PROPERTY:pybind11::module,INTERFACE_INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${test_target} PUBLIC genpybind)
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
    COMMAND Python::Interpreter -m pytest -q
    ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}_test.py
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
