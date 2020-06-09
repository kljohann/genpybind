cmake_minimum_required(VERSION 3.17.3)
project(genpybind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

add_compile_options(
  -Wall -Wextra -Wconversion -pedantic -pedantic-errors
)

find_package(Clang REQUIRED
  HINTS "/usr/lib64/cmake/clang"
)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_library(genpybind SHARED
  src/annotations/annotation.cpp
  src/annotations/literal_value.cpp
  src/annotations/parser.cpp
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

add_custom_target(test)
add_subdirectory(tests)
