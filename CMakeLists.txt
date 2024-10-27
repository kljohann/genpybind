# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

file(STRINGS "src/tool.cpp" PROJECT_VERSION REGEX "^#define GENPYBIND_VERSION_STRING \"[0-9.]+")
# CMake only supports integer components in version numbers, so ignore any suffix.
string(REGEX MATCH "[0-9]+(\\.[0-9])*" PROJECT_VERSION ${PROJECT_VERSION})

project(genpybind LANGUAGES C CXX VERSION ${PROJECT_VERSION})
message(STATUS "Configuring ${PROJECT_NAME} version ${PROJECT_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

option(ENABLE_ASSERTIONS "Enable assertions" ON)
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
if(ENABLE_ASSERTIONS AND NOT uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  add_definitions(-UNDEBUG)
endif()

add_compile_options(
  -Wall -Wextra -Wconversion -pedantic -pedantic-errors
)

find_package(Clang REQUIRED HINTS "/usr/lib64/cmake/clang")
message(STATUS "Found Clang ${Clang_VERSION}; using ClangConfig.cmake in: ${Clang_DIR}")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}; using LLVMConfig.cmake in: ${LLVM_DIR}")

include(${Clang_DIR}/AddClang.cmake)
include(${LLVM_DIR}/AddLLVM.cmake)

find_program(IWYU_PATH NAMES include-what-you-use iwyu)
if(NOT IWYU_PATH STREQUAL "IWYU_PATH-NOTFOUND")
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
    ${IWYU_PATH}
    -Xiwyu --mapping_file=${CMAKE_CURRENT_SOURCE_DIR}/.iwyu-mapping-file.yaml
    -Xiwyu --transitive_includes_only
    -Xiwyu --quoted_includes_first
    -resource-dir=${LLVM_LIBRARY_DIR}/clang/${Clang_VERSION}
  )
endif()

add_library(genpybind-impl STATIC
  src/annotated_decl.cpp
  src/annotations/annotation.cpp
  src/annotations/literal_value.cpp
  src/annotations/parser.cpp
  src/decl_context_graph.cpp
  src/decl_context_graph_builder.cpp
  src/decl_context_graph_processing.cpp
  src/diagnostics.cpp
  src/expose.cpp
  src/inspect_graph.cpp
  src/instantiate_annotated_templates.cpp
  src/instantiate_default_arguments.cpp
  src/lookup_context_collector.cpp
  src/pragmas.cpp
  src/sort_decls.cpp
  src/string_utils.cpp
  src/visible_decls.cpp
)

# Use same compiler flags as LLVM.
llvm_update_compile_flags(genpybind-impl)

# Link against libLLVM.so, if available.
if(LLVM_LINK_LLVM_DYLIB)
  set(llvm_libs LLVM)
else()
  llvm_map_components_to_libnames(llvm_libs support)
endif()

target_link_libraries(genpybind-impl PUBLIC ${llvm_libs})
clang_target_link_libraries(genpybind-impl PUBLIC clangTooling)
target_include_directories(genpybind-impl
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_include_directories(genpybind-impl
  SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS}
)
target_compile_definitions(genpybind-impl
  PUBLIC LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
)

add_executable(genpybind-tool src/tool.cpp)
add_executable(genpybind::genpybind-tool ALIAS genpybind-tool)
llvm_update_compile_flags(genpybind-tool)
target_link_libraries(genpybind-tool PRIVATE genpybind-impl)

install(TARGETS genpybind-tool
  EXPORT genpybindTargets
  RUNTIME
)

add_library(genpybind INTERFACE)
add_library(genpybind::genpybind ALIAS genpybind)
target_include_directories(genpybind
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
  INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
install(
  TARGETS genpybind
  EXPORT genpybindTargets
)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/public/genpybind
  TYPE INCLUDE
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  tools/genpybindConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/genpybindConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/genpybind)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/genpybindConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  EXPORT genpybindTargets
  FILE genpybindTargets.cmake
  NAMESPACE genpybind::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/genpybind
)
install(
  FILES
  "${CMAKE_CURRENT_BINARY_DIR}/genpybindConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/genpybindConfigVersion.cmake"
  tools/genpybind.cmake
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/cmake/genpybind")

include("tools/genpybind.cmake")
add_custom_target(test)
add_subdirectory(tests)
