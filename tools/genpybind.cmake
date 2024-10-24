# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  message(WARNING "genpybind depends on compile_commands.json! \
  Continuing with CMAKE_EXPORT_COMPILE_COMMANDS set to YES.")
  set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
endif()

# genpybind_add_module(<target-name>
#                      HEADER <header-file>
#                      [LINK_LIBRARIES <targets>...]
#                      [NUM_BINDING_FILES <count>]
#                      <pybind11_add_module-args>...)
# Creates a pybind11 module target based on auto-generated bindings for
# the given header file.  If specified, the generated code is split into
# several intermediate files to take advantage of parallel builds.
# <header-file> is evaluated relative to the source directory.
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

  get_filename_component(
    ARG_HEADER ${ARG_HEADER} ABSOLUTE
    BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  )

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
    DEPENDS genpybind::genpybind-tool
    IMPLICIT_DEPENDS CXX ${ARG_HEADER}
    COMMAND $<TARGET_FILE:genpybind::genpybind-tool>
    ARGS -p ${CMAKE_BINARY_DIR} --module-name ${target_name} ${ARG_HEADER}
    ${output_args}
    COMMENT "Analyzing ${ARG_HEADER}"
    VERBATIM
  )

  pybind11_add_module(${target_name} ${bindings} ${ARG_UNPARSED_ARGUMENTS})
  target_link_libraries(
    ${target_name} PRIVATE ${ARG_LINK_LIBRARIES} genpybind::genpybind
  )
endfunction()
