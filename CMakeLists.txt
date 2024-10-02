# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
project(genpybind LANGUAGES C CXX)

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

include(AddLLVM)

find_program(IWYU_PATH NAMES include-what-you-use iwyu)
if(NOT IWYU_PATH STREQUAL "IWYU_PATH-NOTFOUND")
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
    ${IWYU_PATH}
    -Xiwyu --mapping_file=${CMAKE_CURRENT_SOURCE_DIR}/genpybind.imp
    -Xiwyu --transitive_includes_only
    -Xiwyu --quoted_includes_first
    -resource-dir=${LLVM_LIBRARY_DIR}/clang/${Clang_VERSION}
  )
endif()

add_library(genpybind SHARED
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
llvm_update_compile_flags(genpybind)

# Link against libLLVM.so, if available.
if(LLVM_LINK_LLVM_DYLIB)
  set(llvm_libs LLVM)
else()
  llvm_map_components_to_libnames(llvm_libs support)
endif()

target_link_libraries(genpybind PUBLIC ${llvm_libs} clang-cpp)
target_include_directories(genpybind
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  # PUBLIC $<INSTALL_INTERFACE:...>
)
target_include_directories(genpybind
  SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS}
)
target_compile_definitions(genpybind
  PUBLIC LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
)

add_executable(genpybind-tool
  src/tool.cpp
)

llvm_update_compile_flags(genpybind-tool)

target_link_libraries(genpybind-tool PRIVATE genpybind)

add_subdirectory(extern/pybind11)

add_library(genpybind-runtime INTERFACE)
target_include_directories(genpybind-runtime
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime>
  # INTERFACE $<INSTALL_INTERFACE:...>
)

add_custom_target(test)
add_subdirectory(tests)
