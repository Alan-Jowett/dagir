# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(dagir_vcpkg_test LANGUAGES CXX)

find_package(DagIR CONFIG REQUIRED)

add_executable(test_find_package test_find_package.cpp)
target_link_libraries(test_find_package PRIVATE dagir::dagir)
