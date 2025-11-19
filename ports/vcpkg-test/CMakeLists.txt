# SPDX-License-Identifier: MIT
# © DagIR Contributors. All rights reserved.

cmake_minimum_required(VERSION 3.20)
project(dagir_vcpkg_test LANGUAGES CXX)

# Require C++20 for the test build so installed DagIR headers compile correctly.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(DagIR CONFIG REQUIRED)

add_executable(test_find_package test_find_package.cpp)
target_link_libraries(test_find_package PRIVATE dagir::dagir)
