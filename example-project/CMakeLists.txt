# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
project(genpybind-example LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

find_package(genpybind REQUIRED CONFIG)

add_library(readme-impl SHARED src/readme.cpp)
target_link_libraries(readme-impl PUBLIC genpybind::genpybind)
genpybind_add_module(
  readme MODULE
  LINK_LIBRARIES readme-impl
  NUM_BINDING_FILES 1
  HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/readme.h
)
