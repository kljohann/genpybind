cmake_minimum_required(VERSION 3.17.3)
project(genpybind LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 14)
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

find_package(Clang REQUIRED
  HINTS "/usr/lib64/cmake/clang"
)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

find_program(IWYU_PATH NAMES include-what-you-use iwyu)
if(NOT IWYU_PATH STREQUAL "IWYU_PATH-NOTFOUND")
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
    ${IWYU_PATH}
    -Xiwyu --mapping_file=${CMAKE_CURRENT_SOURCE_DIR}/genpybind.imp
    -Xiwyu --transitive_includes_only
    -Xiwyu --quoted_includes_first
  )
endif()

add_library(genpybind SHARED
  src/annotated_decl.cpp
  src/annotations/annotation.cpp
  src/annotations/literal_value.cpp
  src/annotations/parser.cpp
  src/decl_context_collector.cpp
  src/decl_context_graph.cpp
  src/decl_context_graph_builder.cpp
  src/decl_context_graph_processing.cpp
  src/diagnostics.cpp
  src/expose.cpp
  src/inspect_graph.cpp
  src/instantiate_annotated_templates.cpp
  src/string_utils.cpp
  src/visible_decls.cpp
)

# Use same compiler flags as LLVM.
llvm_update_compile_flags(genpybind)

target_link_libraries(genpybind PUBLIC LLVM clang-cpp)
target_include_directories(genpybind
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  # PUBLIC $<INSTALL_INTERFACE:...>
)
target_include_directories(genpybind
  SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS}
)

add_executable(genpybind-tool
  src/tool.cpp
)

target_link_libraries(genpybind-tool PRIVATE genpybind)

add_subdirectory(extern/pybind11)

add_library(genpybind-runtime INTERFACE)
target_include_directories(genpybind-runtime
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/runtime>
  # INTERFACE $<INSTALL_INTERFACE:...>
)

add_custom_target(test)
add_subdirectory(tests)
