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
    # Do not FORCE the cache value so parent projects can override it.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "")
  endif()
endif()

# Apple: enable @rpath for test executables convenience
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

# -----------------------------
# Options
# -----------------------------
# Default tests/samples ON when this is the top-level project, otherwise OFF.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(_dagir_default_build_tests ON)
  set(_dagir_default_samples ON)
else()
  set(_dagir_default_build_tests OFF)
  set(_dagir_default_samples OFF)
endif()

option(DAGIR_BUILD_TESTS "Build DagIR unit tests (downloads Catch2 via FetchContent if not found)" ${_dagir_default_build_tests})
option(DAGIR_SAMPLES "Build DagIR sample code" ${_dagir_default_samples})

# Install developer convenience scripts (pre-commit hook) only for the top-level project
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  include(cmake/format.cmake OPTIONAL)
endif()

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
  file(GLOB DAGIR_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp"
  )

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

    # Ensure there is always at least one CTest entry for the test executable
    # (some CMake/Catch2 setups may not create entries in certain environments).
    add_test(NAME dagir_tests COMMAND dagir_tests)
  else()
    message(WARNING "DAGIR_BUILD_TESTS=ON but no test sources were found under tests/")
  endif()
endif()

# -----------------------------
# Build sample
# -----------------------------
if(DAGIR_SAMPLES)
  # Fetch and patch the BDD libraries

  # Include our custom FetchAndPatch module
  include(cmake/FetchAndPatch.cmake)

  # Fetch and patch TeDDy library from GitHub
  fetch_and_patch(DecisionDiagrams
      GIT_REPOSITORY https://github.com/MichalMrena/DecisionDiagrams.git
      GIT_TAG a5b73864106d98ae407095033e75b0bc29fc22af
      PATCHES_DIR ${CMAKE_SOURCE_DIR}/patches/teddy
  )

  # Fetch and patch CUDD library from GitHub
  fetch_and_patch(CUDD
      GIT_REPOSITORY https://github.com/cuddorg/cudd.git
      GIT_TAG b7b1ca9f35d86e2437ba35be9e5335222c524da0
      PATCHES_DIR ${CMAKE_SOURCE_DIR}/patches/cudd
  )


  # Auto-discover samples: each folder under `samples/` that contains a
  # `main.cpp` will produce an executable named after the folder.
  file(GLOB SAMPLE_MAIN_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/samples/*/main.cpp")

  foreach(_main IN LISTS SAMPLE_MAIN_FILES)
    # _main looks like "samples/<name>/main.cpp"
    get_filename_component(_sample_dir ${_main} DIRECTORY)
    get_filename_component(_sample_name ${_sample_dir} NAME)
    string(REGEX REPLACE "[/\\]+" "_" _target_name ${_sample_name})

    add_executable(${_target_name} ${CMAKE_CURRENT_SOURCE_DIR}/${_main})
    target_compile_features(${_target_name} PRIVATE cxx_std_20)
    target_link_libraries(${_target_name} PRIVATE dagir::dagir)
    target_include_directories(${_target_name} PRIVATE samples/include)

    # If external BDD libraries were fetched (e.g., TeDDy), link their CMake targets
    # into the sample so their include directories and compile options are available.
    if(TARGET teddy::teddy)
      target_link_libraries(${_target_name} PRIVATE teddy::teddy)
    endif()

    # If the sample provides a local `include/` folder, add it as a PRIVATE
    # include directory. Use an absolute path so includes resolve properly.
    set(_sample_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_sample_dir}/include")
    if(EXISTS ${_sample_include_dir})
      target_include_directories(${_target_name} PRIVATE ${_sample_include_dir})
    endif()

    if(MSVC)
      target_compile_options(${_target_name} PRIVATE /W4 /wd4996 /permissive-)
    else()
      target_compile_options(${_target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    set_target_properties(${_target_name} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
  endforeach()
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
