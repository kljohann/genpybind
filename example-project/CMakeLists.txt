# SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

set(PYBIND11_NEWPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)
find_package(genpybind CONFIG REQUIRED)

genpybind_add_module(
  genpybind_example MODULE
  HEADER include/readme.h
  src/readme.cpp
)
target_include_directories(genpybind_example PUBLIC include)

install(TARGETS genpybind_example LIBRARY DESTINATION .)
