# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

find_package(GTest 1.10 CONFIG)
if(NOT GTest_FOUND)
  message(WARNING "GTest not found, skipping unit tests")
else()
  add_executable(genpybind-tests)
  llvm_update_compile_flags(genpybind-tests)
  target_link_libraries(genpybind-tests
    PRIVATE genpybind-impl GTest::gmock GTest::gmock_main)
  file(GLOB_RECURSE test_files CONFIGURE_DEPENDS "*_test.cpp")
  target_sources(genpybind-tests PRIVATE ${test_files})

  add_custom_target(genpybind-tests-run
    COMMAND $<TARGET_FILE:genpybind-tests>
    COMMENT "Running unit tests"
    VERBATIM
  )
  add_dependencies(test genpybind-tests-run)
endif()

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
    --param genpybind-includes=$<TARGET_PROPERTY:genpybind,INTERFACE_INCLUDE_DIRECTORIES>
    --param FileCheck=${FILECHECK_COMMAND}
    ${CMAKE_CURRENT_SOURCE_DIR}/lit
    COMMENT "Running lit tests"
    VERBATIM
  )
  add_dependencies(test lit-tests-run)
endif()

find_package(Python 3.9 COMPONENTS Interpreter Development.Module)
find_package(pybind11 2.10 CONFIG)

if(NOT Python_FOUND)
  message(WARNING "Python not found, skipping integration tests")
elseif(NOT pybind11_FOUND)
  message(WARNING "pybind11 not found, skipping integration tests")
else()
  add_subdirectory(integration)
endif()
