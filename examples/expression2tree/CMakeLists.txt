# SPDX-License-Identifier: MIT
# © DagIR Contributors. All rights reserved.

cmake_minimum_required(VERSION 3.21)
project(expression2tree_sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Example CMake which fetches the dagir repository and builds this sample
include(FetchContent)
FetchContent_Declare(
  dagir
  GIT_REPOSITORY https://github.com/Alan-Jowett/dagir.git
  GIT_TAG main
)
FetchContent_MakeAvailable(dagir)

# The DagIR headers are header-only and provided under the fetched
# ${dagir_SOURCE_DIR}/include path. We create an executable and add
# include directories to make the dagir headers available.
add_executable(expression2tree_main main.cpp)

target_include_directories(expression2tree_main PRIVATE
  ${dagir_SOURCE_DIR}/include
)

# The example uses only the standard library and DagIR headers (header-only).
# If you want to build examples that use external libraries (e.g. TeDDy or CUDD),
# you should find_package() or add the appropriate FetchContent blocks and
# link the required libraries here.

message(STATUS "Added example target 'expression2tree_main'")
