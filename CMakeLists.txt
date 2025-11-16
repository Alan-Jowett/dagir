# SPDX-License-Identifier: MIT
# © DagIR Contributors. All rights reserved.
#
# DagIR - Cross-platform CMake configuration
# Header-only library with optional unit tests (Catch2).
# Works on Windows, Linux, and macOS.

cmake_minimum_required(VERSION 3.20)

# -----------------------------
# Project
# -----------------------------
project(DagIR
  VERSION 0.1.0
  DESCRIPTION "Header-only C++20 library for read-only external DAG views, traversal, and IR rendering"
  HOMEPAGE_URL "https://github.com/alan-jowett/dagir"
  LANGUAGES CXX)

# Global cross-platform settings helpful for consumers that link shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# On MSVC, opt into the newer conformant mode and a reasonable default runtime linkage.
if(MSVC)
  # Prefer the latest conformance and diagnostics where available.
  add_compile_options(/permissive- /Zc:__cplusplus)
  # Leave runtime selection to the parent unless not set.
  if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    # Default to MultiThreadedDLL on MSVC 2019+; consumers can override.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
  endif()
endif()

# Apple: enable @rpath for test executables convenience
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

# -----------------------------
# Options
# -----------------------------
option(DAGIR_BUILD_TESTS "Build DagIR unit tests (downloads Catch2 via FetchContent if not found)" OFF)

# -----------------------------
# Library (header-only)
# -----------------------------
add_library(dagir INTERFACE)
add_library(dagir::dagir ALIAS dagir)

target_compile_features(dagir INTERFACE cxx_std_20)

# Public include directory (users include <dagir/...>)
target_include_directories(dagir
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Consumers can layer their own warnings; remain neutral here.

# -----------------------------
# Install (headers only)
# -----------------------------
include(GNUInstallDirs)

install(TARGETS dagir
  EXPORT DagIRTargets)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# Export targets so downstream projects can import via find_package with a manual config,
# or by using the exported targets file directly.
install(EXPORT DagIRTargets
  NAMESPACE dagir::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DagIR)

# -----------------------------
# Testing (optional, cross-platform)
# -----------------------------
if(DAGIR_BUILD_TESTS)
  include(CTest)
  enable_testing()

  # Prefer an existing Catch2 v3; otherwise fetch it.
  find_package(Catch2 3 QUIET CONFIG)

  if(NOT Catch2_FOUND)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
      catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.6.0   # pin to a known version
    )
    # Work around shallow clone timestamp issues on older CMake/Windows machines.
    cmake_policy(PUSH)
    if(POLICY CMP0135)
      cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_MakeAvailable(catch2)
    cmake_policy(POP)
  endif()

  # Gather test sources (portable glob; users can add more files).
  file(GLOB DAGIR_TEST_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/tests/*.cpp"
    "${PROJECT_SOURCE_DIR}/tests/*_test.cpp")
  message(STATUS "DagIR: Collected test sources: ${DAGIR_TEST_SOURCES}")

  if(DAGIR_TEST_SOURCES)
    add_executable(dagir_tests ${DAGIR_TEST_SOURCES})
    target_compile_features(dagir_tests PRIVATE cxx_std_20)
    target_link_libraries(dagir_tests PRIVATE
      dagir::dagir
      Catch2::Catch2WithMain)

    # Cross-platform warning levels for tests (non-fatal if unsupported).
    if(MSVC)
      target_compile_options(dagir_tests PRIVATE /W4 /wd4996 /permissive-)
    else()
      target_compile_options(dagir_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    # On macOS make sure executables can find dependent dylibs via rpath if needed
    if(APPLE)
      set_target_properties(dagir_tests PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        MACOSX_RPATH ON)
    endif()

    # Register tests using Catch2's discovery
    include(Catch)
    catch_discover_tests(dagir_tests)
  else()
    message(WARNING "DAGIR_BUILD_TESTS=ON but no test sources were found under tests/")
  endif()
endif()

# -----------------------------
# Developer convenience targets
# -----------------------------
add_custom_target(dagir_headers
  COMMAND ${CMAKE_COMMAND} -E echo "DagIR headers at: ${CMAKE_CURRENT_SOURCE_DIR}/include"
  VERBATIM)

# -----------------------------
# Usage notes (for packagers)
# -----------------------------
# Downstream can:
#   find_package(DagIR CONFIG QUIET)  # if you later ship a Config file
# or add_subdirectory() this repo.
#
# For now we export targets only; if you want a full Config package,
# add a DagIRConfig.cmake.in and use CMakePackageConfigHelpers to install it.
