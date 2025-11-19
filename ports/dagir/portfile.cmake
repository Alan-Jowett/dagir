# SPDX-License-Identifier: MIT
include(vcpkg_common_functions)

# Fetch source from GitHub. Update REF and SHA512 to the desired release tag before
# submitting the port upstream into vcpkg.
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO Alan-Jowett/dagir
  REF v0.0.1
  SHA512 F46563BA13C13A3DC5A21F191B834AA5669D52C642503729036F6E192AEB8C3F47CB628DC5C5FF23CE372F767BA25A7E0FFDACEE7E85DF72A5114AE242EEA1CF
)

# Configure and install. DagIR is header-only so we disable tests and samples
# to keep the build minimal inside vcpkg.
vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
  OPTIONS
    -DDAGIR_BUILD_TESTS=OFF
    -DDAGIR_SAMPLES=OFF
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_INSTALL_INCLUDEDIR=include
)

vcpkg_install_cmake()

# Provide a minimal CMake package config if upstream did not install one.
set(_targets_file "${CURRENT_PACKAGES_DIR}/lib/cmake/DagIR/DagIRTargets.cmake")
if(NOT EXISTS "${_targets_file}")
  file(WRITE "${_targets_file}"
    "add_library(dagir INTERFACE)\n"
    "target_include_directories(dagir INTERFACE \"$<INSTALL_INTERFACE:include>\")\n"
    "add_library(dagir::dagir ALIAS dagir)\n"
  )
endif()

file(WRITE "${CURRENT_PACKAGES_DIR}/lib/cmake/DagIR/DagIRConfig.cmake"
  "include(\"$${CMAKE_CURRENT_LIST_DIR}/DagIRTargets.cmake\")\n"
)

# Optionally install samples when the "samples" feature is enabled.
if(VCPKG_FEATURE_FLAGS)
  list(FIND VCPKG_FEATURE_FLAGS "samples" _has_samples)
else()
  # Older vcpkg versions expose selected features via the FEATURES variable
  if(DEFINED FEATURES)
    list(FIND FEATURES "samples" _has_samples)
  endif()
endif()

if(NOT _has_samples EQUAL -1)
  message(STATUS "vcpkg: installing samples because 'samples' feature is enabled")
  file(COPY "${SOURCE_PATH}/samples" DESTINATION "${CURRENT_PACKAGES_DIR}/share/dagir/")
endif()

# End of portfile. The duplicate/sourceforge code has been removed.
