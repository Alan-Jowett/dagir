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
# Default tests/examples ON when this is the top-level project, otherwise OFF.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(_dagir_default_build_tests ON)
  set(_dagir_default_examples ON)
else()
  set(_dagir_default_build_tests OFF)
  set(_dagir_default_examples OFF)
endif()

# Apple - TeDDy doesn't build correctly with Clang
if (APPLE)
  set(_dagir_default_examples OFF)
endif()

option(DAGIR_BUILD_TESTS "Build DagIR unit tests (downloads Catch2 via FetchContent if not found)" ${_dagir_default_build_tests})
option(DAGIR_EXAMPLES "Build DagIR sample code" ${_dagir_default_examples})
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

# Configure coverage flags
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Enabling code coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
endif()

# Add sanitizer options
# These sanitizers help detect various runtime errors during development and testing:
# - ASAN: Detects memory errors (buffer overflows, use-after-free, etc.)
# - UBSAN: Detects undefined behavior (signed integer overflow, etc.)
# - MSAN: Detects uninitialized memory reads (Clang only)
# - TSAN: Detects data races in multi-threaded code
#
# Usage examples:
#   cmake -DENABLE_ASAN=ON ..          # AddressSanitizer
#   cmake -DENABLE_UBSAN=ON ..         # UndefinedBehaviorSanitizer
#   cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..  # Both (compatible)
#
# Note: Some sanitizers are mutually exclusive (ASAN+MSAN, ASAN+TSAN, MSAN+TSAN)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer (Clang only)" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Configure AddressSanitizer (ASan)
if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling AddressSanitizer (ASan) for GCC/Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(STATUS "Enabling AddressSanitizer (ASan) for MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        # MSVC ASan requires specific runtime library
        if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
          set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
        else()
          message(WARNING "CMAKE_MSVC_RUNTIME_LIBRARY already set; ASAN may require MultiThreadedDebug")
        endif()
    else()
        message(WARNING "AddressSanitizer not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# Configure UndefinedBehaviorSanitizer (UBSan)
if(ENABLE_UBSAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling UndefinedBehaviorSanitizer (UBSan) for GCC/Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer -g")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer -g")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=undefined")
    else()
        message(WARNING "UndefinedBehaviorSanitizer not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# Configure MemorySanitizer (MSan) - Clang only
if(ENABLE_MSAN)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "Enabling MemorySanitizer (MSan) for Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fno-omit-frame-pointer -g")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=memory")
    else()
        message(WARNING "MemorySanitizer only supported with Clang, not ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# Configure ThreadSanitizer (TSan)
if(ENABLE_TSAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling ThreadSanitizer (TSan) for GCC/Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -g")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
    else()
        message(WARNING "ThreadSanitizer not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# Sanitizer compatibility check
if((ENABLE_ASAN OR ENABLE_MSAN OR ENABLE_TSAN) AND ENABLE_COVERAGE)
    message(WARNING "Sanitizers and coverage may not work together reliably")
endif()

if(ENABLE_ASAN AND ENABLE_MSAN)
    message(FATAL_ERROR "AddressSanitizer and MemorySanitizer cannot be used together")
endif()

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be used together")
endif()

if(ENABLE_MSAN AND ENABLE_TSAN)
    message(FATAL_ERROR "MemorySanitizer and ThreadSanitizer cannot be used together")
endif()

# Option to treat all warnings as errors (can be disabled by downstream projects)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

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

# Apply warnings-as-errors to consumers of the DagIR interface library when
# the option is enabled. This avoids forcing fetched subprojects or CMake's
# internal try_compile checks to inherit /WX or -Werror globally.
if(WARNINGS_AS_ERRORS)
  # Enforce warnings-as-errors for project-owned test targets only.
  # We avoid propagating /WX or -Werror via the `dagir` INTERFACE target because
  # that forces downstream consumers and fetched third-party template-heavy
  # code (e.g. TeDDy/CUDD) to build with Werror which can break examples.
  # Individual consumers can opt into -Werror themselves by checking the
  # `WARNINGS_AS_ERRORS` option when integrating this project.
  message(STATUS "WARNINGS_AS_ERRORS=ON: tests will be built with warnings-as-errors")
endif()

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
if(DAGIR_EXAMPLES)
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


  # Auto-discover examples: each folder under `examples/` that contains a
  # `main.cpp` will produce an executable named after the folder.
  file(GLOB SAMPLE_MAIN_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/*/main.cpp")

  foreach(_main IN LISTS SAMPLE_MAIN_FILES)
    # _main looks like "examples/<name>/main.cpp"
    get_filename_component(_sample_dir ${_main} DIRECTORY)
    get_filename_component(_sample_name ${_sample_dir} NAME)
    string(REGEX REPLACE "[/\\]+" "_" _target_name ${_sample_name})

    add_executable(${_target_name} ${CMAKE_CURRENT_SOURCE_DIR}/${_main})
    target_compile_features(${_target_name} PRIVATE cxx_std_20)
    target_link_libraries(${_target_name} PRIVATE dagir::dagir)

    # If external BDD libraries were fetched (e.g., TeDDy), link their CMake targets
    # into the sample so their include directories and compile options are available.
    if(TARGET teddy::teddy)
      target_link_libraries(${_target_name} PRIVATE teddy::teddy)
    endif()
    if(TARGET cudd::cudd)
      target_link_libraries(${_target_name} PRIVATE cudd::cudd)
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
# If both examples and tests are enabled, add CTest tests that run the sample
# executables over the expressions and compare their output to the expected
# files under tests/regression_tests/expression_* directories.
if(DAGIR_EXAMPLES AND DAGIR_BUILD_TESTS)
  # Gather expressions
  file(GLOB SAMPLE_EXPRESSIONS "${CMAKE_CURRENT_SOURCE_DIR}/tests/regression_tests/expressions/*.expr")

  if(SAMPLE_EXPRESSIONS)
    include(CTest)

    set(_sample_test_out_dir "${CMAKE_BINARY_DIR}/_sample_test_outputs")
    file(MAKE_DIRECTORY ${_sample_test_out_dir})

    # expression2tree: backends and expected extensions
    set(_tree_backends dot json mermaid)

    foreach(_expr IN LISTS SAMPLE_EXPRESSIONS)
      get_filename_component(_expr_name ${_expr} NAME_WE)

      foreach(_backend IN LISTS _tree_backends)
        if(_backend STREQUAL "mermaid")
          set(_ext md)
        else()
          set(_ext ${_backend})
        endif()

        set(_expected "${CMAKE_CURRENT_SOURCE_DIR}/tests/regression_tests/expression_tree_${_backend}/${_expr_name}.${_ext}")
        if(EXISTS ${_expected})
          set(_out "${_sample_test_out_dir}/expression2tree_${_expr_name}_${_backend}.${_ext}")

          add_test(NAME sample_expression2tree_${_expr_name}_${_backend}
            COMMAND ${CMAKE_COMMAND}
              -DPROG=$<TARGET_FILE:expression2tree>
              -DARG0=${CMAKE_CURRENT_SOURCE_DIR}//tests/regression_tests/expressions/${_expr_name}.expr
              -DARG1=${_backend}
                -DEXPECTED=${_expected}
                -DBINARY_OUT=${_out}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_and_capture.cmake)
        endif()
      endforeach()
    endforeach()

    # expression2bdd: use teddy library to match the expected outputs
    set(_bdd_backends dot json mermaid)

    foreach(_expr IN LISTS SAMPLE_EXPRESSIONS)
      get_filename_component(_expr_name ${_expr} NAME_WE)

      foreach(_backend IN LISTS _bdd_backends)
          if(_backend STREQUAL "mermaid")
            set(_ext md)
          else()
            set(_ext ${_backend})
          endif()

          set(_expected "${CMAKE_CURRENT_SOURCE_DIR}/tests/regression_tests/expression_bdd_${_backend}/${_expr_name}.${_ext}")
          if(EXISTS ${_expected})

            # For BDD backends we support both teddy (TeDDy) and cudd (CUDD) when
            # the corresponding target is available. Use the library name as ARG1.
            # Prefer `teddy` if both targets are present to preserve previous behavior.
            set(_bdd_arg_list)
            if(TARGET teddy::teddy)
              list(APPEND _bdd_arg_list teddy)
            endif()
            if(TARGET cudd::cudd)
              list(APPEND _bdd_arg_list cudd)
            endif()

            # If no backend targets were fetched/available, fall back to `teddy`
            # to avoid changing behavior when TeDDy is present as before.
            if(NOT _bdd_arg_list)
              list(APPEND _bdd_arg_list teddy)
            endif()

            foreach(_bdd_impl IN LISTS _bdd_arg_list)
              set(_out "${_sample_test_out_dir}/expression2bdd_${_expr_name}_${_backend}_${_bdd_impl}.${_ext}")

              add_test(NAME sample_expression2bdd_${_expr_name}_${_backend}_${_bdd_impl}
                COMMAND ${CMAKE_COMMAND}
                  -DPROG=$<TARGET_FILE:expression2bdd>
                  -DARG0=${CMAKE_CURRENT_SOURCE_DIR}/tests/regression_tests/expressions/${_expr_name}.expr
                  -DARG1=${_bdd_impl}
                  -DARG2=${_backend}
                    -DEXPECTED=${_expected}
                    -DBINARY_OUT=${_out}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_and_capture.cmake)
            endforeach()

          endif()
        endforeach()
    endforeach()
  endif()
endif()
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
